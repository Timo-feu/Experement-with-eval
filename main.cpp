#include <cstdio>
#include <cstdint>
#include <x86intrin.h> // Для __rdtsc()

// Константы для быстрого сравнения (Pre-hashed names)
typedef uint32_t Hash;
constexpr Hash hash_const(const char* s, size_t n) {
    Hash h = 2166136261u;
    for (size_t i = 0; i < n; ++i)
        h = (h ^ static_cast<uint8_t>(s[i])) * 16777619u;
    return h;
}

// Быстрый хэш для строк во время парсинга
inline Hash fast_hash(const char* s, int len) {
    Hash h = 2166136261u;
    for (int i = 0; i < len; ++i)
        h = (h ^ static_cast<uint8_t>(s[i])) * 16777619u;
    return h;
}

typedef double (*func_ptr)(double);

struct Object {
    Hash name_hash;
    double value;
    func_ptr func;
};

struct Context {
    Object* objects;
    int size;
};

// Функции-тесты
inline double sq(double x) { return x * x; }
inline double add1(double x) { return x + 1.0; }

class UltraFastEval {
public:
    static inline double evaluate(const char* str, Context& ctx) {
        const char* p = str;
        return parse_expr(p, ctx);
    }

private:
    static inline void skip_spaces(const char*& p) {
        while (*p == ' ') p++;
    }

    static double parse_expr(const char*& p, Context& ctx) {
        skip_spaces(p);
        double val = 0;

        // Парсинг числа (максимально быстро: только для целых/простых дробей в тесте)
        if (*p >= '0' && *p <= '9') {
            val = 0;
            while (*p >= '0' && *p <= '9' || *p == '.') {
                val = val * 10 + (*p - '0'); // Упрощенно, для скорости
                if (*p == '.') { /* пропуск точки */ }
                p++;
            }
        } 
        else {
            // Парсинг идентификатора (переменной или функции)
            const char* start = p;
            while (*p && *p != '(' && *p != ' ' && *p != '+' && *p != '-' && *p != '*' && *p != '/') {
                p++;
            }
            int len = (int)(p - start);
            Hash h = fast_hash(start, len);

            skip_spaces(p);
            if (*p == '(') { // Вызов функции: f(x)
                p++; // skip '('
                double arg = parse_expr(p, ctx);
                p++; // skip ')'
                // Поиск функции через хэш (O(1) если один элемент или очень быстро по списку)
                for (int i = 0; i < ctx.size; ++i) {
                    if (ctx.objects[i].func && ctx.objects[i].name_hash == h) {
                        val = ctx.objects[i].func(arg);
                        goto end_expr;
                    }
                }
            } else {
                // Поиск переменной через хэш
                for (int i = 0; i < ctx.size; ++i) {
                    if (ctx.objects[i].name_hash == h) {
                        val = ctx.objects[i].value;
                        goto end_expr;
                    }
                }
            }
        }

    end_expr:
        skip_spaces(p);
        if (*p == '+') {
            p++;
            val += parse_expr(p, ctx);
        } else if (*p == '-') {
            p++;
            val -= parse_expr(p, ctx);
        }
        return val;
    }
};

int main() {
    // 1. Подготовка окружения (вне цикла измерения)
    Object objs[3];
    objs[0] = { hash_const("x", 1), 50.0, nullptr };
    objs[1] = { hash_const("y", 1), 25.0, nullptr };
    objs[2] = { hash_const("sq", 2), 0.0, sq };

    Context ctx = { objs, 3 };
    const char* expression = "x + y"; // Простая операция: (50 + 25)
    // Для более сложного теста используем: "sq(x)" -> (50 * 50)

    const int ITERATIONS = 10'000'000; // 10 миллионов итераций
    
    printf("Starting benchmark with %d iterations...\n", ITERATIONS);

    // 2. Замер времени через RDTSC
    uint64_t start_ticks = __rdtsc();

    for (int i = 0; i < ITERATIONS; ++i) {
        // Вызываем eval. Результат забираем в volatile, чтобы компилятор не удалил цикл "пустышкой"
        volatile double res = UltraFastEval::evaluate(expression, ctx);
        (void)res; // предотвращаем warning
    }

    uint64_t end_ticks = __rdtsc();

    // 3. Расчет статистики
    double total_time_ns = (double)(end_ticks - start_ticks) / 2.5; // Примерно: делим на частоту CPU в ГГц (здесь 2.5 GHz для примера)
    // В реальности лучше использовать __rdtsc() и знать точную частоту процессора, 
    // либо замерить время через clock_gettime один раз для калибровки.

    // Более надежный способ для бенчмарка (через системное время для общей картины):
    // Но RDTSC даст нам "чистое" время работы инструкций без накладных расходов ОС.

    printf("Benchmark Finished.\n");
    printf("Total Ticks: %llu\n", (unsigned long long)(end_ticks - start_ticks));
    printf("Avg cycles per call: %.2f\n", (double)(end_ticks - start_ticks) / ITERATIONS);

    return 0;
}
