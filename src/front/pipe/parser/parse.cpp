#include "front/pipe/parser/parse.h"

#include "front/pipe/parser/Context.h"
#include "front/pipe/parser/grammar/grammar.h"
#include "front/pipe/parser/grammar/parse.h"
#include "front/pipe/parser/lexer/lex.yy.h"

#include "front/pipe/ast/Expressions.h"  // IWYU pragma: keep
#include "front/pipe/ast/Sources.h"      // IWYU pragma: keep
#include "front/pipe/ast/Stages.h"       // IWYU pragma: keep

#include "front/common/source/require_at.h"

#include "util/verify.h"

namespace lsql::front::pipe::parse {

namespace {

class PipeFlexLexer : public pipe_FlexLexer {
 public:
    explicit PipeFlexLexer(std::string query) : query(std::move(query)) { this->in(this->query); }

    void setParser(void* parser) { m_parser = parser; }
    void setParseContext(Context* ctx) { m_parse_context = ctx; }

    void run() {
        verify(m_parse_context);
        pipeSetLocPtr(&span);
        int token = 0;

        while ((token = pipe_FlexLexer::pipe_lex()) != 0) {
            if (token == 0) {
                continue;
            }

            tokens.emplace_back(YYText());
            requireAt(token != -1, span, "lexer failed at token '{}'", tokens.back());

            Token t{
                .code = token,
                .text = tokens.back().c_str(),
                .span = span,
            };

            PipeParser(m_parser, token, t, m_parse_context);
            checkParser();

            if (token == TOKEN_EOF) {
                break;
            }
        }

        PipeParser(m_parser, 0, {.code = 0, .text = "", .span = {}}, m_parse_context);
        checkParser();
    }

 private:
    void checkParser() {
        if (m_parse_context->has_error) {
            throwAt(m_parse_context->error_span, "parsing failed");
        }
    }

    std::string query;
    void* m_parser = nullptr;
    Context* m_parse_context = nullptr;
    std::deque<std::string> tokens;
    SourceSpan span{};
};

}  // namespace

ast::Program parse(std::string query) {
    Context ctx;
    void* parser = PipeParserAlloc(malloc);

    PipeFlexLexer lexer(std::move(query));
    lexer.setParser(parser);
    lexer.setParseContext(&ctx);

    lexer.run();

    PipeParserFree(parser, free);

    if (ctx.has_error) {
        throw cpptrace::runtime_error("parsing failed");
    }

    return std::move(ctx.program);
}

}  // namespace lsql::front::pipe::parse
