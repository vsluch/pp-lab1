#include <iostream>
#include <locale.h>
#include <vector>
#include <chrono>
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


void *task(void *args) {
    TaskArgs* t = (TaskArgs*)args;

    double *sum = new double;
    *sum = 0;
    for (int i = 0; i < t->n; i++) {
        double x_i = t->a + (i + 0.5) * t->h;
        *sum += f(x_i);
    }
    return sum; // нет умножения на h, потом нужно будет умножать итоговую сумму
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


int main() {
    setlocale(LC_ALL, "Russian");

    int n = input_int(2000000000, "Введите кол-во разбиений: ");
    int p_main = input_int(8, "Введите число потоков для основного вычисления: ");
    int p_max = input_int(24, "Ввведите максимальное число потоков для тестирования: ");
    

    double h = (GLOBAL_B - GLOBAL_A) / n;  // шаг
    int* segments = new int[p_main];    // отрезки для потоков

    int k = n / p_main;
    for (int i = 0; i < p_main; i++) {
        segments[i] = k;
    }
    for (int i = 1; i <= n % p_main; i++) {
        segments[i-1]++;
    }


    double* value_borders = new double[p_main + 1];  // граница значений для потоков
    value_borders[0] = GLOBAL_A;
    for (int i = 1; i < p_main; i++) {
        value_borders[i] = value_borders[i-1] + (double)(segments[i] * h);
    }
    value_borders[p_main] = GLOBAL_B;

    
    auto start_time = chrono::steady_clock::now();  // стартовое время для замера

    double result_sum = 0;
    vector<TaskArgs> args(p_main);
    vector<pthread_t> threads(p_main);
    for (int i = 0; i < p_main; i++) {
        args[i].a = value_borders[i];
        args[i].b = value_borders[i + 1];
        args[i].h = h;
        args[i].n = segments[i];
        if (pthread_create(&threads[i], NULL, task, &args[i]) != 0) {
            cout << "Ошибка создания потока " << i + 1 << endl << "Завершение работы программы" << endl;
            return 0;
        }
    }
    for (int i = 0; i < p_main; i++) {
        void *ret_ptr;
        pthread_join(threads[i], &ret_ptr);
        result_sum += *((double*)ret_ptr);
        delete[](double*)ret_ptr;
    }
    cout << result_sum * h << endl;


    auto end_time = chrono::steady_clock::now(); // окончание замера времени
    auto running_time = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    cout << running_time << endl;
        


    delete[] segments;
    delete[] value_borders;
    segments = nullptr;
    value_borders = nullptr;

    return 0;
}