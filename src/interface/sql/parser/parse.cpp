#include "interface/sql/parser/parse.h"

#include "core/verify.h"

#include "interface/sql/parser/Context.h"

#include "interface/sql/parser/grammar/parse.h"
#include "interface/sql/parser/grammar/sql_grammar.h"

#include "interface/sql/parser/lexer/lex.yy.h"

namespace lsql::iface::sql::parse {

namespace {

class SQLFlexLexer : public yyFlexLexer {
 public:
    explicit SQLFlexLexer(std::istream* input) : yyFlexLexer(input) {}

    void setParser(void* parser) { m_parser = parser; }
    void setParseContext(Context* ctx) { m_parse_context = ctx; }

    void run() {
        verify(m_parse_context);

        int token = 0;

        while ((token = yyFlexLexer::yylex()) != 0) {
            if (token == 0) {
                continue;
            }

            Token t{
                .code = token,
                .text = strdup(YYText()),
            };

            Parse(m_parser, token, t, m_parse_context);

            if (token == TOKEN_EOF) {
                break;
            }
        }

        Parse(m_parser, 0, {.code = 0, .text = ""}, m_parse_context);
    }

 private:
    void* m_parser = nullptr;
    Context* m_parse_context = nullptr;
};

}  // namespace

ast::Program parse(std::istream& is) {
    Context ctx;
    void* parser = ParseAlloc(malloc);

    SQLFlexLexer lexer(&is);
    lexer.setParser(parser);
    lexer.setParseContext(&ctx);

    lexer.run();

    ParseFree(parser, free);

    if (ctx.has_error) {
        throw std::runtime_error("parsing failed");
    }

    return std::move(ctx.program);
}

}  // namespace lsql::iface::sql::parse
