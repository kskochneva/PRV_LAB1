#include <iostream>//ввод-вывод
#include <vector>// для матрицы
#include <chrono>//для времени
#include <boost/thread.hpp>//многопоточность
#include <atomic>//для непрерывности операци
#include <mutex>//только один поток остальные ждутт
#include <thread>//для потоков
#include <iomanip>//для setprecision
#include <random>   // для mt19937

// Глобальный баланс (общий ресурс)
//1000 это просто пример
int balance_unsync = 1000;        // без синхронизации
std::atomic<int> balance_atomic(1000);  // с atomic
int balance_mutex = 1000;          // с mutex
std::mutex mtx;                    // мьютекс для защиты

//std::atomic Использует специальные инструкции процессора
//Не использует блокировки
//Подходит для простых операций (+, -, =)Только для простых типов (int, bool, pointer)
//std::mutex
//Поток "захватывает" мьютекс
//Другие потоки ждут
//Медленнее (переключение контекста)
//Риск deadlock'а
//

// Количество транзакций на клиента
const int TRANSACTIONS_PER_CLIENT = 100000;

// Генератор случайных чисел для каждого потока
// у каждого потока будет своя собственная копия переменной.
// КАЖДЫЙ поток имеет СВОЙ генератор
thread_local std::mt19937 gen(std::random_device{}());//// Создает генератор
thread_local std::uniform_int_distribution<> amount_dist(1, 100);//числа буду от 0 до 100
thread_local std::uniform_int_distribution<> type_dist(0, 1);//это будет пополнение или снятие

// Функция для несинхронизированного доступа
void client_unsync(int id, int& local_balance, int& anomalies) {
    for (int i = 0; i < TRANSACTIONS_PER_CLIENT; i++) {
        int amount = amount_dist(gen);//сам генератоп
        bool is_deposit = type_dist(gen);//тоже генратор случанйости

        // Читаем баланс
        int current = balance_unsync;//задан выше



        // Изменяем баланс
        if (is_deposit) {
            balance_unsync = current + amount;
        } else {
            balance_unsync = current - amount;
        }

        // Проверяем корректность (баланс не должен быть отрицательным)
        if (balance_unsync < 0) {
            anomalies++;
        }
    }
}

// Функция для atomic
void client_atomic(int id, int& local_balance, int& anomalies) {
	//каждый поток должен иметь свой счетчик аномалий

	for (int i = 0; i < TRANSACTIONS_PER_CLIENT; i++) {
        int amount = amount_dist(gen);
        bool is_deposit = type_dist(gen);

        if (is_deposit) {
            balance_atomic.fetch_add(amount);
        } else {
            // Проверяем, что баланс не станет отрицательным
            int current = balance_atomic.load();
            if (current >= amount) {
                balance_atomic.fetch_sub(amount);
            }
        }

        // Проверяем корректность
        if (balance_atomic.load() < 0) {
            anomalies++;
        }
    }
}

// Функция с mutex
void client_mutex(int id, int& local_balance, int& anomalies) {
    for (int i = 0; i < TRANSACTIONS_PER_CLIENT; i++) {
        int amount = amount_dist(gen);
        bool is_deposit = type_dist(gen);

        std::lock_guard<std::mutex> lock(mtx);

        if (is_deposit) {
            balance_mutex += amount;
        } else {
            if (balance_mutex >= amount) {
                balance_mutex -= amount;
            }
        }

        // Проверяем корректность
        if (balance_mutex < 0) {
            anomalies++;
        }
    }
}

// Функция для замера времени выполнения
template<typename Func>//позволяет писать код, который работает с любыми типами данных, без дублирования кода
//Это означает: "Функция measure_time может принимать любую функцию func с любым набором параметров"
double measure_time(Func func, int num_threads, const std::string& name) {
    std::vector<std::thread> threads;
    std::vector<int> local_balances(num_threads, 0);
    std::vector<int> anomalies(num_threads, 0);

    auto start = std::chrono::high_resolution_clock::now();
    //threads - вектор, который хранит все созданные потоки

    //emplace_back() - создает объект потока прямо в векторе (без копирования)

    //Можно представить как "добавить новый поток в конец списка"
    // Создаем потоки
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(func, i, std::ref(local_balances[i]), std::ref(anomalies[i]));
    }


    // Ждем завершения
    for (auto& t : threads) {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    double time = std::chrono::duration<double>(end - start).count();

    // Считаем общее количество аномалий
    int total_anomalies = 0;
    for (int a : anomalies) {
        total_anomalies += a;
    }

    std::cout << std::setw(15) << name << " | "
              << std::setw(10) << time << " s | "
              << std::setw(10) << total_anomalies << " anomalies\n";

    return time;
}

int main() {
    std::vector<int> thread_counts = {2, 4, 8};

    std::cout << "========================================================\n";
    std::cout << "BANK ACCOUNT TRANSACTIONS PERFORMANCE COMPARISON\n";
    std::cout << "========================================================\n";
    std::cout << "Initial balance: 1000\n";
    std::cout << "Transactions per client: " << TRANSACTIONS_PER_CLIENT << "\n\n";

    std::cout << "Threads | Method          | Time       | Result\n";
    std::cout << "--------|-----------------|------------|-----------------\n";

    for (int num_threads : thread_counts) {
        std::cout << std::setw(6) << num_threads << " | \n";

        // Без синхронизации
        balance_unsync = 1000;
        double time_unsync = measure_time(client_unsync, num_threads, "No sync");

        // Atomic
        balance_atomic = 1000;
        double time_atomic = measure_time(client_atomic, num_threads, "Atomic");

        // Mutex
        balance_mutex = 1000;
        double time_mutex = measure_time(client_mutex, num_threads, "Mutex");

        std::cout << "        | Final balances: "
                  << "No sync=" << balance_unsync << ", "
                  << "Atomic=" << balance_atomic << ", "
                  << "Mutex=" << balance_mutex << "\n\n";
    }


    return 0;
}
