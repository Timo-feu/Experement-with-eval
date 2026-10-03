#include <iostream>

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

        T& operator[](const char* key) {
            for (int i = 0; i < vars.size(); i++) {
                if (strcmp(vars[i].name, key) == 0) {
                    return vars[i].value;
                }
            };
            vars.push_back(Variables {key, T()});
            return vars.back().value;
        } 
    };

    struct Variables {
        const char* name;
        T value;
    }
}


int main() {
    return 0;
}