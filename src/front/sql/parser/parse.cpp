#include "front/sql/parser/parse.h"

#include "front/sql/parser/Context.h"
#include "front/sql/parser/grammar/grammar.h"
#include "front/sql/parser/grammar/parse.h"
#include "front/sql/parser/lexer/lex.yy.h"

#include "front/common/source/require_at.h"

#include "util/verify.h"

namespace lsql::front::sql::parse {

namespace {

class SQLFlexLexer : public sql_FlexLexer {
 public:
    explicit SQLFlexLexer(std::string query) : query(std::move(query)) { this->in(this->query); }

    void setParser(void* parser) { m_parser = parser; }
    void setParseContext(Context* ctx) { m_parse_context = ctx; }

    void run() {
        verify(m_parse_context);
        sqlSetLocPtr(&span);
        int token = 0;

        while ((token = sql_FlexLexer::sql_lex()) != 0) {
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

            SqlParser(m_parser, token, t, m_parse_context);
            checkParser();

            if (token == TOKEN_EOF) {
                break;
            }
        }

        SqlParser(m_parser, 0, {.code = 0, .text = "", .span = {}}, m_parse_context);
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
    void* parser = SqlParserAlloc(malloc);

    SQLFlexLexer lexer(std::move(query));
    lexer.setParser(parser);
    lexer.setParseContext(&ctx);

    lexer.run();

    SqlParserFree(parser, free);

    if (ctx.has_error) {
        throw cpptrace::runtime_error("parsing failed");
    }

    return std::move(ctx.program);
}

}  // namespace lsql::front::sql::parse
