#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cstdlib>

enum { IDLE, RESUME, STOP };

std::mutex m;
std::condition_variable markerCv, mainCv;
std::vector<int> arr, command;
bool started = false;
int n, blockedCount = 0;

void sleep5()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
}

void printArray()
{
    for (int x : arr)
        std::cout << x << ' ';
    std::cout << '\n';
}

void marker(int id)
{
    std::unique_lock<std::mutex> lk(m);
    markerCv.wait(lk, [] { return started; });
    std::srand(id);
    int marked = 0;
    while (true)
    {
        int index = std::rand() % n;
        if (arr[index] == 0)
        {
            sleep5();
            arr[index] = id;
            ++marked;
            lk.unlock();
            sleep5();
            lk.lock();
        }
        else
        {
            std::cout << "marker " << id << ": marked " << marked << ", cannot mark index " << index << '\n';
            ++blockedCount;
            mainCv.notify_one();
            markerCv.wait(lk, [id] { return command[id] != IDLE; });
            int cmd = command[id];
            command[id] = IDLE;
            if (cmd == STOP)
            {
                for (int& x : arr)
                    if (x == id)
                        x = 0;
                return;
            }
        }
    }
}

int main()
{
    int k;
    std::cout << "Array size: ";
    std::cin >> n;
    std::cout << "Markers count: ";
    std::cin >> k;
    if (n <= 0 || k <= 0)
        return 1;

    arr.assign(n, 0);
    command.assign(k + 1, IDLE);
    std::vector<bool> alive(k + 1, true);
    std::vector<std::thread> threads;
    for (int i = 1; i <= k; ++i)
        threads.emplace_back(marker, i);

    std::unique_lock<std::mutex> lk(m);
    started = true;
    markerCv.notify_all();

    for (int left = k; left > 0; --left)
    {
        mainCv.wait(lk, [left] { return blockedCount == left; });
        printArray();

        int id;
        std::cout << "Marker to stop: ";
        while (!(std::cin >> id) || id < 1 || id > k || !alive[id])
        {
            std::cin.clear();
            std::cin.ignore(1000, '\n');
        }

        alive[id] = false;
        command[id] = STOP;
        markerCv.notify_all();
        lk.unlock();
        threads[id - 1].join();
        lk.lock();
        printArray();

        blockedCount = 0;
        for (int i = 1; i <= k; ++i)
            if (alive[i])
                command[i] = RESUME;
        markerCv.notify_all();
    }

    return 0;
}
