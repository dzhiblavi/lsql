#include "front/pipe/parser/parse.h"

#include "front/pipe/ast/Expressions.h"  // IWYU pragma: keep
#include "front/pipe/ast/Sources.h"      // IWYU pragma: keep
#include "front/pipe/ast/Stages.h"       // IWYU pragma: keep

#include "front/pipe/parser/Context.h"
#include "front/pipe/parser/grammar/grammar.h"
#include "front/pipe/parser/grammar/parse.h"
#include "front/pipe/parser/lexer/lex.yy.h"

#include "util/verify.h"

#include <cpptrace/exceptions.hpp>

namespace lsql::front::pipe::parse {

namespace {

class PipeFlexLexer : public pipe_FlexLexer {
 public:
    explicit PipeFlexLexer(std::istream* input) : pipe_FlexLexer(input) {}

    void setParser(void* parser) { m_parser = parser; }
    void setParseContext(Context* ctx) { m_parse_context = ctx; }

    void run() {
        verify(m_parse_context);

        int token = 0;

        while ((token = pipe_FlexLexer::pipe_lex()) != 0) {
            if (token == 0) {
                continue;
            }

            tokens.emplace_back(YYText());
            Token t{
                .code = token,
                .text = tokens.back().c_str(),
            };

            PipeParser(m_parser, token, t, m_parse_context);

            if (token == TOKEN_EOF) {
                break;
            }
        }

        PipeParser(m_parser, 0, {.code = 0, .text = ""}, m_parse_context);
    }

 private:
    void* m_parser = nullptr;
    Context* m_parse_context = nullptr;
    std::deque<std::string> tokens;
};

}  // namespace

ast::Program parse(std::istream& is) {
    Context ctx;
    void* parser = PipeParserAlloc(malloc);

    PipeFlexLexer lexer(&is);
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
