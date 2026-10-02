#include <iostream>
#include <locale.h>
#include <vector>
#include <chrono>
#include <format>
#include <thread>
#include "include/pthread.h"

using namespace std;


double f(double x) {
    return (2.0 * x - sin(x)) / (10.0 + x * x);
}

// границы интервала
const double GLOBAL_A = 1.0;
const double GLOBAL_B = 5.0;

struct TaskArgs {
    int n;
    double a;
    double b;
    double h;
};

struct Result {    // для возврата результата и времени posix методом
    double result;
    chrono::milliseconds time;
};


void *task(void *args) {
    TaskArgs* t = (TaskArgs*)args;

    double *sum = new double;
    *sum = 0;
    for (int i = 0; i < t->n; i++) {
        double x_i = t->a + (i + 0.5) * t->h;
        *sum += f(x_i);
    }
    return sum; // нет умножения на h, потом умножается итоговая сумма
}


void task_std(int n, double a, double h, double *result) {
    double sum = 0;
    for (int i = 0; i < n; i++) {
        double x_i = a + (i + 0.5) * h;
        sum += f(x_i);
    }
    *result = sum;
}


int input_int(int max_value, const string promt) {
    int result = 0;
    while (true) {
        cout << promt;
        cin >> result;
        if (cin.fail() || result < 1 || result > max_value) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Ошибка ввода. Ожидается целое число от 1 до " << max_value << endl;
        }
        else {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return result;
        }
    }
}


Result posix_threads(int n, int p) {
    double h = (GLOBAL_B - GLOBAL_A) / n;   // шаг
    int* segments = new int[p];             // отрезки для потоков

    int k = n / p;  // первоначальное значение столбцов на поток
    for (int i = 0; i < p; i++) {
        segments[i] = k;
    }
    for (int i = 1; i <= n % p; i++) {
        segments[i - 1]++;
    }

    double* value_borders = new double[p + 1];  // граница значений для потоков
    value_borders[0] = GLOBAL_A;
    for (int i = 1; i < p; i++) {
        value_borders[i] = value_borders[i - 1] + (double)(segments[i - 1] * h);
    }
    value_borders[p] = GLOBAL_B;

    auto start_time = chrono::steady_clock::now();  // стартовое время для замера
    double result_sum = 0;
    vector<TaskArgs> args(p);
    vector<pthread_t> threads(p);
    for (int i = 0; i < p; i++) {
        args[i].a = value_borders[i];
        args[i].b = value_borders[i + 1];
        args[i].h = h;
        args[i].n = segments[i];
        if (pthread_create(&threads[i], NULL, task, &args[i]) != 0) {
            cout << "Ошибка создания потока " << i + 1 << endl;
            return Result(-1, chrono::milliseconds(-1));
        }
    }

    for (int i = 0; i < p; i++) {
        void* ret_ptr;
        pthread_join(threads[i], &ret_ptr);
        result_sum += *((double*)ret_ptr);
        delete[](double*)ret_ptr;
    }
    double result = result_sum * h;

    auto end_time = chrono::steady_clock::now(); // окончание замера времени
    auto running_time = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);

    delete[] segments;
    delete[] value_borders;
    segments = nullptr;
    value_borders = nullptr;

    Result ret(result, running_time);
    return ret;
}


bool is_correct_Result(Result pr) {
    if (pr.result == -1 && pr.time.count() == -1) { return false; }
    return true;
}


int len_int(int n) {
    string str = to_string(n);
    return str.length();
}


void print_results(Result res, int stream) {  // для одного потока
    string streams_str(12 - len_int(stream), ' ');
    cout << stream << streams_str;
    string time_str(14 - len_int(res.time.count() - 2), ' ');
    cout << res.time << time_str;
    string result_str1 = format("{:.5f}", res.result);
    string result_str2(14 - result_str1.length(), ' ');
    cout << result_str1 << result_str2;
}


void print_test_results(vector<Result> results, int stream) {  // для потоков от 1 до stream
    if (results.empty()) { return; }
    int one_thread_time = results[0].time.count();    // время выполнения одним потоком
    cout << "РЕЗУЛЬТАТЫ ТЕСТИРОВАНИЯ" << endl;
    cout << "Потоки      " << "Время, мс     " << "Результат     " << "Ускорение     " << "Эффективность, %" << endl;
    for (int i = 0; i < stream; i++) {
        print_results(results[i], i + 1);
        double boost = (double)one_thread_time / results[i].time.count();
        string boost_str = format("{:.5f}", boost);
        string boost_str2(14 - boost_str.length(), ' ');
        cout << boost_str << boost_str2;
        double effectivenes = (boost / (double)(i + 1)) * 100;
        string eff_str = format("{:.5f}", effectivenes);
        cout << eff_str << endl;
    }
    cout << endl << endl;
}




Result std_threads(int n, int p) 
{
    double h = (GLOBAL_B - GLOBAL_A) / n;   // шаг
    int* segments = new int[p];             // отрезки для потоков

    int k = n / p;  // первоначальное значение столбцов на поток
    for (int i = 0; i < p; i++) {
        segments[i] = k;
    }
    for (int i = 1; i <= n % p; i++) {
        segments[i - 1]++;
    }

    double* value_borders = new double[p + 1];  // граница значений для потоков
    value_borders[0] = GLOBAL_A;
    for (int i = 1; i < p; i++) {
        value_borders[i] = value_borders[i - 1] + (double)(segments[i - 1] * h);
    }
    value_borders[p] = GLOBAL_B;


    auto start_time = chrono::steady_clock::now(); // стартовое время для замера
    double result_sum = 0;
    vector<thread> threads(p);
    double* thread_results = new double[p];
    for (int i = 0; i < p; i++) {
        threads[i] = thread(task_std, segments[i], value_borders[i], h, &thread_results[i]);
    }

    for (int i = 0; i < p; i++) {
        threads[i].join();
        result_sum += thread_results[i];
    }
    double result = result_sum * h;

    auto end_time = chrono::steady_clock::now();
    auto running_time = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);

    delete[] segments;
    delete[] value_borders;
    delete[] thread_results;
    segments = nullptr;
    value_borders = nullptr;
    thread_results = nullptr;

    return Result(result, running_time);
}



int main() {
    setlocale(LC_ALL, "Russian");

    int n = input_int(2000000000, "Введите кол-во разбиений: ");
    int p_main = input_int(8, "Введите число потоков для основного вычисления: ");
    int p_max = input_int(24, "Ввведите максимальное число потоков для тестирования: ");
    
    // основное вычисление posix
    Result posix_result_main = posix_threads(n, p_main);
    if (!is_correct_Result(posix_result_main)) { cout << "Завершение работы программы" << endl; return 0; }
    cout << "Потоки      " << "Время, мс     " << "Результат" << endl;
    print_results(posix_result_main, p_main);


    // тестирование posix
    vector<Result> posix_res_tests(p_max);
    for (int i = 1; i <= p_max; i++) {
        Result res = posix_threads(n, i);
        if (!is_correct_Result(res)) { cout << "Завершение работы программы" << endl; return 0; }
        posix_res_tests[i-1] = res;
    }
    cout << endl << endl;
    print_test_results(posix_res_tests, p_max);



    // основное вычисление std
    Result std_thread_result_main = std_threads(n, p_main);
    if (!is_correct_Result(std_thread_result_main)) { cout << "Завершение рабоы программы" << endl; return 0; }
    cout << "Потоки      " << "Время, мс     " << "Результат" << endl;
    print_results(std_thread_result_main, p_main);

    // тестирование std
    vector<Result> std_res_tests(p_max);
    for (int i = 1; i <= p_max; i++) {
        Result res = std_threads(n, i);
        if (!is_correct_Result(res)) { cout << "Завершение работы программы" << endl; return 0; }
        std_res_tests[i - 1] = res;
    }
    cout << endl << endl;
    print_test_results(std_res_tests, p_max);

    

    return 0;
}