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

    enum class NodeType {Number, Variable, Add, Sub, Mul, Div};

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
            case NodeType::Number:      return T(cur_node.value);
            
            case NodeType::Variable:    return ctx[cur_node.var_name];

            case NodeType::Add:
                return evaluator(cur_node.left, pool, ctx) + evaluator(cur_node.right, pool, ctx);
            

            case NodeType::Sub:
                return evaluator(cur_node.left, pool, ctx) - evaluator(cur_node.right, pool, ctx);
            
            
            case NodeType::Mul:
                return evaluator(cur_node.left, pool, ctx) * evaluator(cur_node.right, pool, ctx);
            
            case NodeType::Div:
                return evaluator(cur_node.left, pool, ctx) / evaluator(cur_node.right, pool, ctx);
        }       
        return T(0.0);
    }
}


int main() {
    return 0;
}