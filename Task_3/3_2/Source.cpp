#include <iostream>
#include <queue>
#include <future>
#include <thread>
#include <chrono>
#include <cmath>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <random>
#include <fstream>
#include <sstream>
#include <vector>
#include <atomic>
#include <algorithm>

// Шаблонный класс Server для обработки задач
template<typename T>
class Server
{
public:
    using Task = std::packaged_task<T()>;

    Server() : stop_flag(false) {}

    void start()
    {
        server_thread = std::thread(&Server::process_tasks, this);
    }

    void stop()
    {
        stop_flag.store(true);
        if (server_thread.joinable())
        {
            server_thread.join();
        }
    }

    size_t add_task(Task task)
    {
        std::unique_lock lock(mut);
        size_t id = next_task_id++;
        std::future<T> result = task.get_future();
        tasks.push({id, std::move(task)});
        results[id] = std::move(result);
        return id;
    }

    T request_result(size_t id)
    {
        std::future<T> result;
        {
            std::unique_lock lock(mut);
            auto it = results.find(id);
            if (it != results.end())
            {
                result = std::move(it->second);
                results.erase(it);
            }
        }
        return result.get();
    }

private:
    void process_tasks()
    {
        while (!stop_flag.load())
        {
            std::unique_lock lock(mut);
            if (!tasks.empty())
            {
                auto [id, task] = std::move(tasks.front());
                tasks.pop();
                lock.unlock();

                task();

                lock.lock();
                task_results.push(id);
            }
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        std::cout << "Server stopped!\n";
    }

    std::mutex mut;
    std::queue<std::pair<size_t, Task>> tasks;
    std::unordered_map<size_t, std::future<T>> results;
    std::queue<size_t> task_results;
    size_t next_task_id = 1;
    std::thread server_thread;
    std::atomic<bool> stop_flag;
};

// Пример задачи: вычисление синуса
double calculate_sin(double x)
{
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return std::sin(x);
}

void client_thread(Server<double>& server, int num_tasks, const std::string& filename)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0, 3.14159265358979323846); // диапазон [0, pi]

    std::ofstream outfile(filename);
    if (!outfile.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    for (int i = 0; i < num_tasks; ++i)
    {
        double x = dis(gen);
        auto task = std::packaged_task<double()>(std::bind(calculate_sin, x));
        size_t id = server.add_task(std::move(task));

        double result = server.request_result(id);
        outfile << id << " " << x << " " << result << '\n';
    }

    outfile.close();
}

// Функция для тестирования
bool test_results(const std::string& filename, double tolerance = 1e-6)
{
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    std::string line;
    while (std::getline(infile, line))
    {
        std::istringstream iss(line);
        size_t id;
        double x, result;
        if (!(iss >> id >> x >> result)) {
            std::cerr << "Failed to parse line: " << line << '\n';
            return false;
        }

        double expected = std::sin(x);
        if (std::abs(result - expected) > tolerance) {
            std::cerr << "Mismatch in results for id " << id << ": expected " << expected << ", got " << result << '\n';
            return false;
        }
    }

    infile.close();
    return true;
}

int main()
{
    std::cout << "Start\n";

    Server<double> server;
    server.start();

    int num_tasks = 10;
    std::thread client1(client_thread, std::ref(server), num_tasks, "client1_results.txt");
    std::thread client2(client_thread, std::ref(server), num_tasks, "client2_results.txt");
    std::thread client3(client_thread, std::ref(server), num_tasks, "client3_results.txt");

    client1.join();
    client2.join();
    client3.join();

    server.stop();

    // Тестирование результатов
    bool test1 = test_results("client1_results.txt", 1e-5);
    bool test2 = test_results("client2_results.txt", 1e-5);
    bool test3 = test_results("client3_results.txt", 1e-5);

    if (test1 && test2 && test3)
    {
        std::cout << "All tests passed!\n";
    }
    else
    {
        std::cerr << "Some tests failed!\n";
    }

    std::cout << "End\n";
    return 0;
}
