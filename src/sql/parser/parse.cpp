#include "sql/parser/parse.h"

#include "core/verify.h"

#include "sql/parser/Context.h"

#include "sql/parser/grammar/parse.h"
#include "sql/parser/grammar/sql_grammar.h"

#include "sql/parser/lexer/lex.yy.h"

namespace lsql::sql::parse {

namespace {

class SQLFlexLexer : public yyFlexLexer {
 public:
    explicit SQLFlexLexer(std::istream* input) : yyFlexLexer(input) {}

    void setParser(void* parser) { m_parser = parser; }
    void setParseContext(lsql::sql::parse::Context* ctx) { m_parse_context = ctx; }

    void run() {
        verify(m_parse_context);

        int token = 0;

        while ((token = yyFlexLexer::yylex()) != 0) {
            if (token == 0) {
                continue;
            }

            lsql::sql::parse::Token t{
                .code = token,
                .text = strdup(YYText()),
            };

            Parse(m_parser, token, t, m_parse_context);

            if (token == TOKEN_EOF) {
                break;
            }
        }

        Parse(m_parser, 0, {.code = 0, .text = ""}, m_parse_context);
        verify(m_parse_context->root != nullptr);
    }

 private:
    void* m_parser = nullptr;
    lsql::sql::parse::Context* m_parse_context = nullptr;
};

}  // namespace

std::unique_ptr<ast::Node> parse(std::istream& is) {
    sql::parse::Context ctx;
    void* parser = ParseAlloc(malloc);

    SQLFlexLexer lexer(&is);
    lexer.setParser(parser);
    lexer.setParseContext(&ctx);

    lexer.run();

    ParseFree(parser, free);

    if (ctx.has_error) {
        throw std::runtime_error("parsing failed");
    }

    verify(ctx.root != nullptr);
    return std::move(ctx.root);
}

}  // namespace lsql::sql
