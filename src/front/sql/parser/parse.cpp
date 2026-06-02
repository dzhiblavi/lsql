#include "front/sql/parser/parse.h"

#include "util/verify.h"

#include "front/sql/parser/Context.h"

#include "front/sql/parser/grammar/grammar.h"
#include "front/sql/parser/grammar/parse.h"

#include "front/sql/parser/lexer/lex.yy.h"

namespace lsql::front::sql::parse {

namespace {

class SQLFlexLexer : public sql_FlexLexer {
 public:
    explicit SQLFlexLexer(std::istream* input) : sql_FlexLexer(input) {}

    void setParser(void* parser) { m_parser = parser; }
    void setParseContext(Context* ctx) { m_parse_context = ctx; }

    void run() {
        verify(m_parse_context);

        int token = 0;

        while ((token = sql_FlexLexer::sql_lex()) != 0) {
            if (token == 0) {
                continue;
            }

            tokens.emplace_back(YYText());
            Token t{
                .code = token,
                .text = tokens.back().c_str(),
            };

            SqlParser(m_parser, token, t, m_parse_context);

            if (token == TOKEN_EOF) {
                break;
            }
        }

        SqlParser(m_parser, 0, {.code = 0, .text = ""}, m_parse_context);
    }

 private:
    void* m_parser = nullptr;
    Context* m_parse_context = nullptr;
    std::deque<std::string> tokens;
};

}  // namespace

ast::Program parse(std::istream& is) {
    Context ctx;
    void* parser = SqlParserAlloc(malloc);

    SQLFlexLexer lexer(&is);
    lexer.setParser(parser);
    lexer.setParseContext(&ctx);

    lexer.run();

    SqlParserFree(parser, free);

    if (ctx.has_error) {
        throw std::runtime_error("parsing failed");
    }

    return std::move(ctx.program);
}

}  // namespace lsql::front::sql::parse
