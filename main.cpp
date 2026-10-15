#include <vector>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <chrono>
#include <string>

namespace SoulEngine
{
    struct T
    {
        enum class Forms
        {
            Number,
            Object
        };
        Forms form;
        union
        {
            double number;
            void *obj_ptr;
        };
        const char *class_type = nullptr;

        T() : form(Forms::Number), number(0.0) {};
        T(double num) : form(Forms::Number), number(num) {};
        T(void *obj) : form(Forms::Object), obj_ptr(obj) {};
        T(void *obj, const char *type) : form(Forms::Object), obj_ptr(obj), class_type(type) {}

        T &operator=(double num)
        {
            form = Forms::Number;
            number = num;
            return *this;
        }
        T &operator=(void *obj)
        {
            form = Forms::Object;
            obj_ptr = obj;
            return *this;
        }

        T operator+(const T &t) const
        {
            if ((t.form == Forms::Number) && (form == Forms::Number))
                return T(number + t.number);
            return T(0.0);
        }
        T operator-(const T &t) const
        {
            if (form == Forms::Number && t.form == Forms::Number)
                return T(number - t.number);
            return T(0.0);
        }
        T operator*(const T &t) const
        {
            if (form == Forms::Number && t.form == Forms::Number)
                return T(number * t.number);
            return T(0.0);
        }
        T operator/(const T &t) const
        {
            if (form == Forms::Number && t.form == Forms::Number)
                return T(number / t.number);
            return T(0.0);
        }
    };

    struct Variable
    {
        const char *name;
        T value;
    };

    enum NodeType
    {
        NodeNumber,
        NodeVariable,
        NodeAdd,
        NodeSub,
        NodeMul,
        NodeDiv,
        NodeMemberAccess
    };
    enum TokenType
    {
        TokNumber,
        TokVariable,
        TokOperator,
        TokLeftParent,
        TokRightParen,
        TokDot
    };

    struct Node
    {
        NodeType type;
        double value = 0.0;
        const char *var_name = nullptr;
        size_t lenght = 0;
        int left = -1;
        int right = -1;
    };

    struct Token
    {
        TokenType type;
        double number_value;
        const char *start_ptr;
        size_t lenght;
        char op_char;
    };

    class Context
    {
    private:
        std::vector<Variable> vars;

    public:
        std::vector<Token> token_buffer;
        std::vector<Node> pool_buffer;
        std::vector<int> node_stack_buffer;
        std::vector<char> op_stack_buffer;

        Context() = default;
        ~Context() = default;

        T &operator[](const char *key)
        {
            for (size_t i = 0; i < vars.size(); i++)
            {
                size_t j = 0;
                while (vars[i].name[j] != '\0' && key[j] != '\0' && vars[i].name[j] == key[j])
                {
                    j++;
                }

                if (vars[i].name[j] == '\0')
                {
                    char k = key[j];
                    if (k == '\0' || k == ' ' || k == '.' || k == '\t' || k == '+' || k == '*' || k == '-' || k == '/')
                    {
                        return vars[i].value;
                    }
                }
            }
            vars.push_back(Variable{key, T()});
            return vars.back().value;
        }
    };

    typedef double (*MethodWrapper)(void *);
    struct RegisteredMethod
    {
        const char *class_type;
        const char *method_name;
        MethodWrapper func_ptr;
    };
    inline std::vector<RegisteredMethod> method_registry;

    template <typename ClassType, double (ClassType::*Method)()>
    double invokeWrapper(void *obj_ptr) { return (static_cast<ClassType *>(obj_ptr)->*Method)(); }
    T evaluator(const int nod_indx, const std::vector<Node> &pool, Context &ctx)
    {
        if (nod_indx == -1)
            return T(0.0);
        const Node &cur_node = pool[nod_indx];

        switch (cur_node.type)
        {
        case NodeNumber:
            return T(cur_node.value);
        case NodeVariable:
            return ctx[cur_node.var_name];
        case NodeAdd:
            return evaluator(cur_node.left, pool, ctx) + evaluator(cur_node.right, pool, ctx);
        case NodeSub:
            return evaluator(cur_node.left, pool, ctx) - evaluator(cur_node.right, pool, ctx);
        case NodeMul:
            return evaluator(cur_node.left, pool, ctx) * evaluator(cur_node.right, pool, ctx);
        case NodeDiv:
            return evaluator(cur_node.left, pool, ctx) / evaluator(cur_node.right, pool, ctx);

        case NodeMemberAccess:
        {
            T obj = evaluator(cur_node.left, pool, ctx);
            if (obj.form != T::Forms::Object)
                return T(0.0);

            const Node &method_node = pool[cur_node.right];
            const char *target_method = method_node.var_name;
            size_t method_len = method_node.lenght;

            for (size_t i = 0; i < method_registry.size(); i++)
            {
                const auto &m = method_registry[i];
                if (std::strcmp(m.class_type, obj.class_type) == 0 &&
                    std::strlen(m.method_name) == method_len &&
                    std::strncmp(m.method_name, target_method, method_len) == 0)
                {
                    return T(m.func_ptr(obj.obj_ptr));
                }
            }
            return T(0.0);
        }
        }
        return T(0.0);
    }

    static void Lexer(const char *src, std::vector<Token> &tokens)
    {
        tokens.clear();
        size_t i = 0;
        Token curNode;

        while (src[i] != '\0')
        {
            char c = src[i];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
            {
                i++;
                continue;
            }

            if (std::isdigit(c))
            {
                curNode.type = TokNumber;
                curNode.start_ptr = &src[i];
                char *endPtr;
                curNode.number_value = std::strtod(&src[i], &endPtr);
                i += (endPtr - &src[i]);
                tokens.push_back(curNode);
                continue;
            }

            if (std::isalpha(c) || c == '_')
            {
                curNode.type = TokVariable;
                curNode.start_ptr = &src[i];
                size_t start_index = i;

                while (src[i] != '\0' && (std::isalnum(src[i]) || src[i] == '_'))
                {
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
            case '.':
                curNode.type = TokOperator;
                curNode.start_ptr = &src[i];
                curNode.op_char = '.';
                tokens.push_back(curNode);
                i++;
                break;
            default:
                i++;
                break;
            }
        }
    }

    void applyOperator(std::vector<Node> &pool, std::vector<int> &nodes, std::vector<char> &ops)
    {
        char op = ops.back();
        ops.pop_back();
        int right_idx = nodes.back();
        nodes.pop_back();
        int left_idx = nodes.back();
        nodes.pop_back();

        NodeType type;
        switch (op)
        {
        case '.':
            type = NodeMemberAccess;
            break;
        case '+':
            type = NodeAdd;
            break;
        case '-':
            type = NodeSub;
            break;
        case '*':
            type = NodeMul;
            break;
        case '/':
            type = NodeDiv;
            break;
        default:
            return;
        }

        Node newNode;
        newNode.type = type;
        newNode.left = left_idx;
        newNode.right = right_idx;
        pool.push_back(newNode);
        nodes.push_back(pool.size() - 1);
    }

    int getPriority(char op)
    {
        switch (op)
        {
        case '*':
        case '/':
            return 2;
        case '+':
        case '-':
            return 1;
        case '.':
            return 3;
        default:
            return 0;
        }
    }

    int parse(const std::vector<Token> &tokens, std::vector<Node> &node_pool, std::vector<int> &node_stack, std::vector<char> &op_stack)
    {
        node_pool.clear();
        node_stack.clear();
        op_stack.clear();

        for (size_t i = 0; i < tokens.size(); i++)
        {
            const Token &tok = tokens[i];
            if (tok.type == TokNumber)
            {
                Node numNode;
                numNode.type = NodeNumber;
                numNode.value = tok.number_value;
                node_pool.push_back(numNode);
                node_stack.push_back(node_pool.size() - 1);
            }
            else if (tok.type == TokVariable)
            {
                Node varNode;
                varNode.type = NodeVariable;
                varNode.var_name = tok.start_ptr;
                varNode.lenght = tok.lenght;
                node_pool.push_back(varNode);
                node_stack.push_back(node_pool.size() - 1);
            }
            else if (tok.type == TokOperator)
            {
                char current_op = tok.op_char;
                while (!op_stack.empty() && getPriority(op_stack.back()) >= getPriority(current_op))
                {
                    applyOperator(node_pool, node_stack, op_stack);
                }
                op_stack.push_back(current_op);
            }
        }
        while (!op_stack.empty())
        {
            applyOperator(node_pool, node_stack, op_stack);
        }
        return node_stack.empty() ? -1 : node_stack.back();
    }

    double eval(const char *expression, Context &ctx)
    {
        Lexer(expression, ctx.token_buffer);
        int root_idx = parse(ctx.token_buffer, ctx.pool_buffer, ctx.node_stack_buffer, ctx.op_stack_buffer);
        return evaluator(root_idx, ctx.pool_buffer, ctx).number;
    }
}

class TestPlayer
{
private:
    double helth = 100;

public:
    double GetHp() { return this->helth; }
    void AddHp(double offset) { this->helth += offset; }
    void MinusHp(double offset) { this->helth -= offset; }
};

class Monster {
    private:
        double helth = 200;
    public: 
        double GetHp() { return this->helth; }
};

static size_t g_heap_allocations = 0;
static size_t g_allocated_bytes = 0;

void *operator new(size_t size)
{
    g_heap_allocations++;
    g_allocated_bytes += size;
    return std::malloc(size);
}
void operator delete(void *memory) noexcept { std::free(memory); }

int main()
{
    SoulEngine::method_registry.push_back({"TestPlayer", "GetHp", &SoulEngine::invokeWrapper<TestPlayer, &TestPlayer::GetHp>});
    SoulEngine::method_registry.push_back({"Monster", "GetHp", &SoulEngine::invokeWrapper<Monster, &Monster::GetHp>});
    SoulEngine::Context ctx;

    ctx.token_buffer.reserve(64);
    ctx.pool_buffer.reserve(64);
    ctx.node_stack_buffer.reserve(32);
    ctx.op_stack_buffer.reserve(32);

    TestPlayer player;
    Monster monster;

    ctx["modifier"] = 5.5;
    ctx["player"] = SoulEngine::T(&player, "TestPlayer");
    ctx["monster"] = SoulEngine::T(&monster, "Monster");

    std::vector<std::string> dynamic_expressions = {
        "2 * 5 + player.GetHp + modifier",
        "(modifier - 10) * 3 + 150",
        "player.GetHp * modifier / 2.5",
        "1000.0 - (player.GetHp + modifier * 2)",
        "modifier + modifier + 50 * 2",
        "player.GetHp / 100 * modifier + 125 - 40000",
        "monster.GetHp + player.GetHp / 34.6"
    };

    std::printf("==================================================\n");
    std::printf("   SoulEngine v2.0 Profiling & Leak Detection    \n");
    std::printf("==================================================\n");
    std::printf("Warm-up and initial memory footprint analysis...\n");

    size_t allocs_before_loop = g_heap_allocations;
    size_t bytes_before_loop = g_allocated_bytes;

    const int ITERATIONS = 10000000;

    std::printf("Running stress-test: %d iterations...\n", ITERATIONS);
    double sum = 0;

    auto start_time = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < ITERATIONS; ++i)
    {
        const std::string& current_expr = dynamic_expressions[i % dynamic_expressions.size()];
        double result = SoulEngine::eval(current_expr.c_str(), ctx);
        sum += result;
    }

    auto end_time = std::chrono::high_resolution_clock::now();

    auto total_time_us = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    double avg_time_ns = (static_cast<double>(total_time_us) * 1000.0) / ITERATIONS;

    size_t dynamic_allocations_during_loop = g_heap_allocations - allocs_before_loop;
    size_t dynamic_bytes_during_loop = g_allocated_bytes - bytes_before_loop;

    std::printf("==================================================\n");
    std::printf("                BENCHMARK RESULTS (v12.4)         \n");
    std::printf("==================================================\n");
    std::printf("Expression       : Dynamic Array Test (5 formulas)\n");
    std::printf("Avg Time per Eval: %f nanoseconds\n", avg_time_ns);
    std::printf("--------------------------------------------------\n");
    std::printf("Heap Allocs in Loop: %zu (Expected: 0)\n", dynamic_allocations_during_loop);
    std::printf("Heap Bytes in Loop : %zu bytes (Expected: 0)\n", dynamic_bytes_during_loop);

    if (dynamic_allocations_during_loop == 0) {std::printf("Memory Leak Status : STEEL SOLID // ZERO LEAKS // 0 BYTES OVERHEAD\n");}
    else {std::printf("Memory Leak Status : WARNING // LEAKS DETECTED\n");}

    std::printf("Checksum Verify    : %f\n", sum / ITERATIONS);
    std::printf("==================================================\n");
    
    return 0;
}