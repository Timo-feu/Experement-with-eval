#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <cctype>

namespace SoulEngine {

    #define VAR(engine, type, name, val) \
        type name = val;                 \
        (engine).bind(#name, &name)

    class BaseAny {
    public:
        virtual ~BaseAny() = default;
        virtual std::type_index getType() const = 0;
    };

    struct T {
        enum class Kind { Number, Object };
        Kind kind;
        union {
            double number;
            BaseAny* obj_ptr;
        };

        T() : kind(Kind::Number), number(0.0) {}
        T(double num) : kind(Kind::Number), number(num) {}
        T(BaseAny* obj) : kind(Kind::Object), obj_ptr(obj) {}

        T operator+(const T& o) const { 
            if(kind == Kind::Number && o.kind == Kind::Number) return number + o.number; 
            throw std::runtime_error("SoulEngine Math Error: Invalid '+' operation"); 
        }
        T operator-(const T& o) const { 
            if(kind == Kind::Number && o.kind == Kind::Number) return number - o.number; 
            throw std::runtime_error("SoulEngine Math Error: Invalid '-' operation"); 
        }
        T operator*(const T& o) const { 
            if(kind == Kind::Number && o.kind == Kind::Number) return number * o.number; 
            throw std::runtime_error("SoulEngine Math Error: Invalid '*' operation"); 
        }
        T operator/(const T& o) const { 
            if(kind == Kind::Number && o.kind == Kind::Number) { 
                if(o.number == 0.0) throw std::runtime_error("SoulEngine Error: Division by zero"); 
                return number / o.number; 
            } 
            throw std::runtime_error("SoulEngine Math Error: Invalid '/' operation"); 
        }
        bool operator==(double val) const { return kind == Kind::Number && number == val; }
    };

    template <typename ClassType>
    class AnyType : public BaseAny {
        union {
            ClassType* ptr;       
            uintptr_t raw_bits;   
        };
        bool is_owned;            
    public:
        AnyType(ClassType* obj, bool owned = false) : is_owned(owned) {
            if constexpr (std::is_arithmetic_v<ClassType>) {
                uintptr_t addr = reinterpret_cast<uintptr_t>(obj);
                raw_bits = addr | (owned ? 1 : 0);
            } else { 
                ptr = obj; 
            }
        }

        ~AnyType() override {
            if constexpr (std::is_arithmetic_v<ClassType>) {
                if ((raw_bits & 1) == 1) { 
                    delete reinterpret_cast<ClassType*>(raw_bits & ~static_cast<uintptr_t>(1)); 
                }
            } else { 
                if (is_owned) delete ptr; 
            }
        }

        ClassType* get() const {
            if constexpr (std::is_arithmetic_v<ClassType>) { 
                return reinterpret_cast<ClassType*>(raw_bits & ~static_cast<uintptr_t>(1)); 
            } else { 
                return ptr; 
            }
        }

        std::type_index getType() const override { 
            return std::type_index(typeid(ClassType)); 
        }
    };

    class Context {
        std::map<std::string, std::unique_ptr<BaseAny>> vars;

    public:
        Context() = default;
        ~Context() = default;

        template <typename ClassType>
        void addReference(std::string name, ClassType* ptr) { 
            vars[name] = std::make_unique<AnyType<ClassType>>(ptr, false); 
        }

        BaseAny* getRaw(std::string name) { 
            auto it = vars.find(name);
            return (it != vars.end()) ? it->second.get() : nullptr; 
        }

        bool find(std::string name) const { 
            return vars.find(name) != vars.end(); 
        }
        
        double& get(std::string name) {
            if (find(name)) { 
                return *(static_cast<AnyType<double>*>(vars[name].get())->get()); 
            }
            double* new_ptr = new double(0.0);
            vars[name] = std::make_unique<AnyType<double>>(new_ptr, true);
            return *new_ptr;
        }
    };

    class BaseInvoker {
    public:
        virtual ~BaseInvoker() = default;
        virtual T invoke(BaseAny* obj) = 0;
    };

    template <typename ClassType, typename MemberType>
    class FieldInvoker : public BaseInvoker {
        MemberType ClassType::*field_ptr;
    public:
        FieldInvoker(MemberType ClassType::*field) : field_ptr(field) {}

        T invoke(BaseAny* obj) override {
            if (obj->getType() != std::type_index(typeid(ClassType))) {
                throw std::runtime_error("SoulEngine Registry Error: Type mismatch during field invocation!");
            }
            ClassType* native = static_cast<AnyType<ClassType>*>(obj)->get();
            if constexpr (std::is_arithmetic_v<MemberType>) { 
                return T(static_cast<double>(native->*field_ptr)); 
            }
            throw std::runtime_error("SoulEngine Error: Unsupported field type in math engine");
        }
    };

    template <typename ClassType, typename ReturnType>
    class MethodInvoker : public BaseInvoker {
        ReturnType (ClassType::*method_ptr)();
    public:
        MethodInvoker(ReturnType (ClassType::*method)()) : method_ptr(method) {}

        T invoke(BaseAny* obj) override {
            if (obj->getType() != std::type_index(typeid(ClassType))) {
                throw std::runtime_error("SoulEngine Registry Error: Type mismatch during method invocation!");
            }
            ClassType* native = static_cast<AnyType<ClassType>*>(obj)->get();
            if constexpr (std::is_arithmetic_v<ReturnType>) { 
                return T(static_cast<double>((native->*method_ptr)())); 
            }
            throw std::runtime_error("SoulEngine Error: Unsupported method return type");
        }
    };

    class ClassRegistry {
        std::map<std::type_index, std::map<std::string, std::unique_ptr<BaseInvoker>>> registry;
    public:
        ClassRegistry() = default;

        template <typename ClassType, typename MemberType>
        void registerField(std::string name, MemberType ClassType::*field) {
            registry[std::type_index(typeid(ClassType))][name] = std::make_unique<FieldInvoker<ClassType, MemberType>>(field);
        }

        template <typename ClassType, typename ReturnType>
        void registerMethod(std::string name, ReturnType (ClassType::*method)()) {
            registry[std::type_index(typeid(ClassType))][name] = std::make_unique<MethodInvoker<ClassType, ReturnType>>(method);
        }

        BaseInvoker* getInvoker(std::type_index type, std::string name) {
            auto t_it = registry.find(type);
            if (t_it != registry.end()) {
                auto m_it = t_it->second.find(name);
                if (m_it != t_it->second.end()) {
                    return m_it->second.get();
                }
            }
            return nullptr;
        }
    };

    class Node {
    public: 
        virtual ~Node() = default;
        virtual T evaluator(Context& ctx) const = 0; 
    };

    class ValNode : public Node {
        double value; 
    public: 
        ValNode(double val) : value(val) {}    
        T evaluator(Context&) const override { return T(value); } 
    };

    class VarNode : public Node {
        std::string name;
    public:
        VarNode(std::string n) : name(n) {}
        T evaluator(Context& ctx) const override {
            if (ctx.find(name)) {
                BaseAny* raw = ctx.getRaw(name);
                if (raw && raw->getType() == std::type_index(typeid(double))) {
                    return T(ctx.get(name));
                }
                return T(raw);
            }
            return T(ctx.get(name));
        } 
    };

    class BinaryNode : public Node {
    protected:
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;
    public:
        BinaryNode(std::unique_ptr<Node> l, std::unique_ptr<Node> r) : left(std::move(l)), right(std::move(r)) {}
    };

    class AddNode : public BinaryNode { public: using BinaryNode::BinaryNode; T evaluator(Context& ctx) const override { return left->evaluator(ctx) + right->evaluator(ctx); } };
    class SubNode : public BinaryNode { public: using BinaryNode::BinaryNode; T evaluator(Context& ctx) const override { return left->evaluator(ctx) - right->evaluator(ctx); } };
    class MulNode : public BinaryNode { public: using BinaryNode::BinaryNode; T evaluator(Context& ctx) const override { return left->evaluator(ctx) * right->evaluator(ctx); } };
    class DivNode : public BinaryNode { public: using BinaryNode::BinaryNode; T evaluator(Context& ctx) const override { return left->evaluator(ctx) / right->evaluator(ctx); } };

    class MemberAccessNode : public Node {
        std::unique_ptr<Node> target;
        std::string member_name;
        ClassRegistry& reg;
    public:
        MemberAccessNode(std::unique_ptr<Node> t, std::string m, ClassRegistry& r) 
            : target(std::move(t)), member_name(m), reg(r) {}

        T evaluator(Context& ctx) const override {
            T obj_res = target->evaluator(ctx);
            if (obj_res.kind != T::Kind::Object) {
                throw std::runtime_error("SoulEngine Runtime Error: Dot operator applied to non-object expression!");
            }
            if (!obj_res.obj_ptr) {
                throw std::runtime_error("SoulEngine Runtime Error: Attempted access on nullptr object!");
            }
            BaseInvoker* invoker = reg.getInvoker(obj_res.obj_ptr->getType(), member_name);
            if (!invoker) {
                throw std::runtime_error("SoulEngine Reflection Error: Member '" + member_name + "' not found in registry!");
            }
            return invoker->invoke(obj_res.obj_ptr);
        }
    };
    enum class TokenType { 
        Number, 
        Variable, 
        Operator, 
        LeftParenthesis, 
        RightParenthesis, 
        Dot 
    };

    struct Token { 
        TokenType type; 
        std::string value; 
    };

    class Lexer {
    public:
        std::vector<Token> tokenize(std::string src) {
            std::vector<Token> tokens; 
            size_t pos = 0;
            
            while (pos < src.size()) {
                char c = src[pos];
                
                if (std::isspace(c)) { pos++; continue; }
                if (c == '+') { tokens.push_back({TokenType::Operator, "+"}); pos++; continue; }
                if (c == '-') { tokens.push_back({TokenType::Operator, "-"}); pos++; continue; }
                if (c == '*') { tokens.push_back({TokenType::Operator, "*"}); pos++; continue; }
                if (c == '/') { tokens.push_back({TokenType::Operator, "/"}); pos++; continue; }
                if (c == '(') { tokens.push_back({TokenType::LeftParenthesis, "("}); pos++; continue; }
                if (c == ')') { tokens.push_back({TokenType::RightParenthesis, ")"}); pos++; continue; }
                if (c == '.') { tokens.push_back({TokenType::Dot, "."}); pos++; continue; }
                
                if (std::isdigit(c)) {
                    std::string num; 
                    while(pos < src.size() && (std::isdigit(src[pos]) || src[pos] == '.')) {
                        num += src[pos++];
                    }
                    tokens.push_back({TokenType::Number, num}); 
                    continue;
                }
                
                if (std::isalpha(c) || c == '_') {
                    std::string str; 
                    while(pos < src.size() && (std::isalnum(src[pos]) || src[pos] == '_')) {
                        str += src[pos++];
                    }
                    tokens.push_back({TokenType::Variable, str}); 
                    continue;
                }
                throw std::runtime_error("SoulEngine Lexer Error: Unknown character in expression!");
            }
            return tokens;
        }
    };

    class Engine {
        Context mem;
        ClassRegistry& reg; 
        std::unique_ptr<Node> root;

        int getPriority(char op) { 
            return (op == '+' || op == '-') ? 1 : ((op == '*' || op == '/') ? 2 : 0); 
        }

        void applyOperator(std::vector<std::unique_ptr<Node>>& nodes, std::vector<char>& ops) {
            if (ops.empty() || nodes.size() < 2) {
                throw std::runtime_error("SoulEngine Parser Error: Invalid expression syntax!");
            }
            char op = ops.back(); 
            ops.pop_back();
            
            auto right = std::move(nodes.back()); nodes.pop_back();
            auto left = std::move(nodes.back()); nodes.pop_back();
            
            if (op == '+') nodes.push_back(std::make_unique<AddNode>(std::move(left), std::move(right)));
            if (op == '-') nodes.push_back(std::make_unique<SubNode>(std::move(left), std::move(right)));
            if (op == '*') nodes.push_back(std::make_unique<MulNode>(std::move(left), std::move(right)));
            if (op == '/') nodes.push_back(std::make_unique<DivNode>(std::move(left), std::move(right)));
        }

    public:
        Engine(ClassRegistry& r) : reg(r) {}
        
        void bind(std::string name, double* ptr) { 
            mem.addReference(name, ptr); 
        }

        template <typename ClassType>
        void bindObject(std::string name, ClassType* ptr) { 
            mem.addReference(name, ptr); 
        }
        
        void pars(std::string expression) {
            std::vector<std::unique_ptr<Node>> nodes; 
            std::vector<char> ops;
            Lexer lex; 
            auto tokens = lex.tokenize(expression);
            
            for (size_t i = 0; i < tokens.size(); ++i) {
                auto const& token = tokens[i];
                
                if (token.type == TokenType::Number) {
                    nodes.push_back(std::make_unique<ValNode>(std::stod(token.value)));
                } 
                else if (token.type == TokenType::Variable) {
                    std::unique_ptr<Node> current_node = std::make_unique<VarNode>(token.value);
                    
                    while (i + 1 < tokens.size() && tokens[i + 1].type == TokenType::Dot) {
                        if (i + 2 >= tokens.size() || tokens[i + 2].type != TokenType::Variable) {
                            throw std::runtime_error("SoulEngine Parser Error: Expected member name after dot operator!");
                        }
                        std::string member = tokens[i + 2].value;
                        i += 2; 
                        current_node = std::make_unique<MemberAccessNode>(std::move(current_node), member, reg);
                    }
                    nodes.push_back(std::move(current_node));
                } 
                else if (token.type == TokenType::Operator) {
                    char op = token.value[0];
                    while (!ops.empty() && getPriority(ops.back()) >= getPriority(op)) {
                        applyOperator(nodes, ops);
                    }
                    ops.push_back(op);
                } 
                else if (token.type == TokenType::LeftParenthesis) { 
                    ops.push_back('(');
                } 
                else if (token.type == TokenType::RightParenthesis) {
                    while (!ops.empty() && ops.back() != '(') {
                        applyOperator(nodes, ops);
                    }
                    if (!ops.empty()) ops.pop_back();
                }
            }
            while (!ops.empty()) applyOperator(nodes, ops);
            if (!nodes.empty()) root = std::move(nodes.back());
        }

        double eval(std::string expr) {
            pars(expr); 
            T res = root->evaluator(mem);
            if (res.kind != T::Kind::Number) {
                throw std::runtime_error("SoulEngine Error: Expression evaluated to an object container, expected number!");
            }
            return res.number;
        }

        Context& getMem() { return mem; }
    };
} 

class Player {
private:
    double hp; 
public:
    Player(double initial_hp) : hp(initial_hp) {}

    double getHp() { return hp; }
    
    double takeDamage() { 
        hp -= 25.0; 
        return hp; 
    }

    double heel() {
        hp+= 25.0;
        return hp;
    }
};

int main() {
    SoulEngine::ClassRegistry registry;
    registry.registerMethod("getHp", &Player::getHp);
    registry.registerMethod("takeDamage", &Player::takeDamage);
    registry.registerMethod("heel", &Player::heel);

    SoulEngine::Engine engine(registry);
    
    VAR(engine, double, modifier, 5.0);

    Player player(100.0);
    engine.bindObject("player", &player);

    try {
        double res_init = engine.eval("player.getHp * 2 + modifier");
        std::cout << "res1:" << res_init << std::endl;

        double res_damaged = engine.eval("player.takeDamage + modifier");
        std::cout << "res2:" << res_damaged << std::endl;

        double res_heeled = engine.eval("player.heel");
        std::cout << "res3:" << res_heeled << std::endl;


    } catch (const std::exception& e) {
        std::cerr << "Пизда, не сработало " << e.what() << std::endl;
    }

    return 0;
}
