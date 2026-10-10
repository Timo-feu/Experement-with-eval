#include <iostream>
#include <vector>

namespace SoulEngine {
    struct T{
        enum class Forms{Number, Object};

        Forms form;

        union {
            double number;
            void* obj_ptr;
        };

        T() : form(Forms::Number), number(0.0) {};
        T(double num) : form(Forms::Number), number(num) {};
        T(void* obj) : form(Forms::Object), obj_ptr(obj) {};

        ~T() = default;

        T& operator=(double num) {
            form = Forms::Number;
            number = num;
            return *this;
        }

        T& operator=(void* obj) {
            form = Forms::Object;
            obj_ptr = obj;
            return *this;
        }

        T operator+(const T* t) const {
            if ((t->form == Forms::Number) && (form == Forms::Number)){
                return T(number + t->number); 
            }
            return T(0.0);
        }
    };

    struct Variable {
        const char* name;
        T value;
    };

    enum NodeType {NodNumber, NodVariable, NodAdd, NodSub, NodMul, NodDiv};

    struct Node {
        NodeType type;

        double value = 0.0;
        const char* var_name = nullptr;

        int left = -1;
        int right = -1;
    };

    class Context {
        private: 
           std::vector<Variable> vars;
           
        public:
            Context() = default;
            ~Context() = default;
            
        T& operator[](const char* key) {
            for (int i = 0; i < vars.size(); i++) {
                if (std::strcmp(vars[i].name, key) == 0) {
                    return vars[i].value;
                }
            };
            vars.push_back(Variable {key, T()});
            return vars.back().value;
        
        }
    };

    T evaluator(const int nod_indx, const std::vector<Node>& pool, Context& ctx) {
        if (nod_indx == -1) return T(0.0);

        const Node& cur_node = pool[nod_indx];

        switch (cur_node.type)
        {
            case NodNumber:      return T(cur_node.value);
            
            case NodVariable:    return ctx[cur_node.var_name];

            case NodAdd:
                return evaluator(cur_node.left, pool, ctx) + evaluator(cur_node.right, pool, ctx);
            

            case NodSub:
                return evaluator(cur_node.left, pool, ctx) - evaluator(cur_node.right, pool, ctx);
            
            
            case NodMul:
                return evaluator(cur_node.left, pool, ctx) * evaluator(cur_node.right, pool, ctx);
            
            case NodDiv:
                return evaluator(cur_node.left, pool, ctx) / evaluator(cur_node.right, pool, ctx);
        }       
        return T(0.0);
    }
    
    enum TokenType {TokType, TokVariable, TokOperator, TokLeftParent, TokRightParen, TokDot};

    struct Token
    {
        TokenType type;
        double number_value;
        const char* start_ptr;
        size_t lenght;
        char op_char;
    };
    
    static std::vector<Token> Lexer(const char* src) { 
        std::vector<Token> tokens;
        size_t i = 0;
        Token curNode;

        while (src[i] != '\0') {
            char c = src[i];

            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                i++;
                continue;
            }

            if (std::isdigit(c)) {
                curNode.type = TokNumber;
                curNode.start_ptr = &src[i];

                char* endPtr;
                curNode.number_value = std::strtod(&src[i], &endPtr);
                i += (endPtr - &src[i]); 

                tokens.push_back(curNode);
                continue;
            }

            if (std::isalpha(c) || c == '_') {
                curNode.type = TokVariable;
                curNode.start_ptr = &src[i];

                size_t start_index = i;
                while (src[i] != '\0' && (std::isalnum(src[i]) || src[i] == '_')) {
                    i++;
                }
                curNode.lenght = i - start_index; 

                tokens.push_back(curNode);
                continue;
            }
            
            switch (c)
            {
                case '+':
                    curNode.type = TokOperator;
                    curNode.start_ptr = &src[i];
                    curNode.op_char = '+';
                    tokens.push_back(curNode);
                    i++;
                    break;
                
                case '-': 
                    curNode.type = TokOperator;
                    curNode.start_ptr = &src[i];
                    curNode.op_char = '-';
                    tokens.push_back(curNode);
                    i++;
                    break;
        
                case '*': 
                    curNode.type = TokOperator;
                    curNode.start_ptr = &src[i];
                    curNode.op_char = '*';
                    tokens.push_back(curNode);
                    i++;
                    break;
                
                case '/': 
                    curNode.type = TokOperator;
                    curNode.start_ptr = &src[i];
                    curNode.op_char = '/';
                    tokens.push_back(curNode);
                    i++;
                    break;

                default:
                    i++;
                    break;
            }
        }
        return tokens;
    }
}

int main() {
    return 0;
}
