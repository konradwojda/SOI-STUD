#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <iostream>

#define SLEEP usleep(1000000 + rand() % 1000000)
#define PRINT(x) std::cout << x << std::endl

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

#include <thread>
#include <mutex>

unsigned int numOfProdEvenWaiting = 0;

unsigned int numOfProdOddWaiting = 0;

unsigned int numOfConsEvenWaiting = 0;

unsigned int numOfConsOddWaiting = 0;

std::mutex mutex;
std::mutex prodEven;
std::mutex prodOdd;
std::mutex consEven;
std::mutex consOdd;

class Buffer
{
    std::vector<int> buffer;

public:

    std::vector<int> Buff()
    {
        return buffer;
    }

    void put(int num)
    {
        buffer.push_back(num);
    }

    int get()
    {
        if (buffer.empty())
        {
            return -1;
        }

        int value = buffer[0];
        buffer.erase(buffer.begin());
        return value;
    }

    int count() const
    {
        return buffer.size();
    }

    int countEven() const
    {
        int counter = 0;

        for (int i = 0; i < buffer.size(); i++)
        {
            if(buffer[i] % 2 == 0)
            {
                counter++;
            }
        }
        return counter;
    }

    int countOdd() const
    {
        return buffer.size() - countEven();
    }


    int peekFirst() const
    {
        if(buffer.empty())
        {
            return -1;
        }
        return buffer[0];
    }
};


Buffer buffer;

bool canProdEven()
{
    if (buffer.countEven() < 10)
    {
        return true;
    }
    return false;
}

bool canProdOdd()
{
    if (buffer.countEven() > buffer.countOdd())
    {
        return true;
    }
    return false;
}

bool canConsEven()
{
    if(buffer.count() >= 3 && buffer.peekFirst() != -1 && buffer.peekFirst() % 2 == 0)
    {
        return true;
    }
    return false;
}

bool canConsOdd()
{
    if (buffer.count() >= 7  && buffer.peekFirst() != -1 && buffer.peekFirst() % 2 != 0)
    {
        return true;
    }
    return false;
}

//prodEven
void* a1(void* arg)
{
    for(int i = 0;; i = (i+2) % 50)
    {
        mutex.lock();

        if (!canProdEven())
        {
            numOfProdEvenWaiting++;
            mutex.unlock();
            prodEven.lock();
            numOfProdEvenWaiting--;
        }
        buffer.put(i);
        std::string s = "A1: Dodano " + std::to_string(i);
        PRINT(s);

        std::cout << "v = { ";
        for (int n : buffer.Buff()) {
            std::cout << n << ", ";
        }
        std::cout << "}; \n";

        if(canProdOdd() && numOfProdOddWaiting > 0)
        {
            prodOdd.unlock();
        }
        else if (canConsEven() && numOfConsEvenWaiting > 0)
        {
            consEven.unlock();
        }
        else if (canConsOdd() && numOfConsOddWaiting > 0)
        {
            consOdd.unlock();
        }
        else
        {
            mutex.unlock();
        }

        SLEEP;
    }
}

//prodOdd
void* a2(void* arg)
{
    for(int i = 1;; i = (i+2) % 50)
    {
        mutex.lock();

        if (!canProdOdd())
        {
            numOfProdOddWaiting++;
            mutex.unlock();
            prodOdd.lock();
            numOfProdOddWaiting--;
        }
        buffer.put(i);
        std::string s = "A2: Dodano " + std::to_string(i);
        PRINT(s);

        std::cout << "v = { ";
        for (int n : buffer.Buff()) {
            std::cout << n << ", ";
        }
        std::cout << "}; \n";

        if(canProdEven() && numOfProdEvenWaiting > 0)
        {
            prodEven.unlock();
        }
        else if (canConsEven() && numOfConsEvenWaiting > 0)
        {
            consEven.unlock();
        }
        else if (canConsOdd() && numOfConsOddWaiting > 0)
        {
            consOdd.unlock();
        }
        else
        {
            mutex.unlock();
        }

        SLEEP;
    }
}

//consEven
void* b1(void* arg)
{
    while(1)
    {
        mutex.lock();
        if(!canConsEven())
        {
            numOfConsEvenWaiting++;
            mutex.unlock();
            consEven.lock();
            numOfConsEvenWaiting--;
        }
        int val = buffer.get();
        std::string s = "B1: Zabrano " + std::to_string(val);
        PRINT(s);

        std::cout << "v = { ";
        for (int n : buffer.Buff()) {
            std::cout << n << ", ";
        }
        std::cout << "}; \n";

        if(canProdEven() && numOfProdEvenWaiting > 0)
        {
            prodEven.unlock();
        }
        else if(canProdOdd() && numOfProdOddWaiting > 0)
        {
            prodOdd.unlock();
        }
        else if (canConsOdd() && numOfConsOddWaiting > 0)
        {
            consOdd.unlock();
        }
        else
        {
            mutex.unlock();
        }
        SLEEP;

    }
}

//consOdd
void* b2(void* arg)
{
    while(1)
    {
        mutex.lock();
        if(!canConsOdd())
        {
            numOfConsOddWaiting++;
            mutex.unlock();
            consOdd.lock();
            numOfConsOddWaiting--;
        }
        int val = buffer.get();
        std::string s = "B2: Zabrano " + std::to_string(val);
        PRINT(s);

        std::cout << "v = { ";
        for (int n : buffer.Buff()) {
            std::cout << n << ", ";
        }
        std::cout << "}; \n";

        if(canProdEven() && numOfProdEvenWaiting > 0)
        {
            prodEven.unlock();
        }
        else if(canProdOdd() && numOfProdOddWaiting > 0)
        {
            prodOdd.unlock();
        }
        else if (canConsEven() && numOfConsEvenWaiting > 0)
        {
            consEven.unlock();
        }
        else
        {
            mutex.unlock();
        }
        SLEEP;

    }
}

int main(int argc, char* argv[])
{
    prodEven.lock();
    prodOdd.lock();
    consEven.lock();
    consOdd.lock();

    srand(time(NULL));
    pthread_t thrds[4];

    if (argc == 1)
    {
        std::cout << "Wybierz scenariusz jako agrument (1-8)" << std::endl;
        return 0;
    }

    // Scenariusz 1 - tylko procudent parzystych
    if ((*argv[1] == '1'))
    {
        pthread_create(&thrds[0], NULL, &a1, NULL);
        pthread_join(thrds[0], NULL);
    }

    // Scenariusz 2 - tylko procudent nieparzystych
    else if ((*argv[1] == '2'))
    {
        pthread_create(&thrds[0], NULL, &a2, NULL);
        pthread_join(thrds[0], NULL);
    }

    // Scenariusz 3 - tylko konsument parzystych
    else if ((*argv[1] == '3'))
    {
        pthread_create(&thrds[0], NULL, &b1, NULL);
        pthread_join(thrds[0], NULL);
    }

    // Scenariusz 4 - tylko konsument nieparzystych
    else if ((*argv[1] == '4'))
    {
        pthread_create(&thrds[0], NULL, &b2, NULL);
        pthread_join(thrds[0], NULL);
    }

    // Scenariusz 5 - producenci parzystych i nieparzystych
    else if ((*argv[1] == '5'))
    {
        pthread_create(&thrds[0], NULL, &a1, NULL);
        pthread_create(&thrds[1], NULL, &a2, NULL);
        pthread_join(thrds[0], NULL);
        pthread_join(thrds[1], NULL);
    }

    // Scenariusz 6 - konsumenci parzystych i nieparzystych
    else if ((*argv[1] == '6'))
    {
        pthread_create(&thrds[0], NULL, &b1, NULL);
        pthread_create(&thrds[1], NULL, &b2, NULL);
        pthread_join(thrds[0], NULL);
        pthread_join(thrds[1], NULL);
    }

    // Scenariusz 7 - producenci i konsumenci
    else if ((*argv[1] == '7'))
    {
        pthread_create(&thrds[0], NULL, &a1, NULL);
        pthread_create(&thrds[1], NULL, &a2, NULL);
        pthread_create(&thrds[2], NULL, &b1, NULL);
        pthread_create(&thrds[3], NULL, &b2, NULL);
        pthread_join(thrds[0], NULL);
        pthread_join(thrds[1], NULL);
        pthread_join(thrds[2], NULL);
        pthread_join(thrds[3], NULL);
    }

    // Scenariusz 8 - zakleszczenie - główny mutex zablokowany na początku
    else if ((*argv[1] == '8'))
    {
        mutex.lock();
        pthread_create(&thrds[0], NULL, &a1, NULL);
        pthread_create(&thrds[1], NULL, &a2, NULL);
        pthread_create(&thrds[2], NULL, &b1, NULL);
        pthread_create(&thrds[3], NULL, &b2, NULL);
        pthread_join(thrds[0], NULL);
        pthread_join(thrds[1], NULL);
        pthread_join(thrds[2], NULL);
        pthread_join(thrds[3], NULL);
    }


    //------------ Po jednym ------------------
    // pthread_create(&thrds[0], NULL, &a1, NULL);
    // pthread_join(thrds[0], NULL);

    // pthread_create(&thrds[0], NULL, &a2, NULL);
    // pthread_join(thrds[0], NULL);

    // pthread_create(&thrds[0], NULL, &b1, NULL);
    // pthread_join(thrds[0], NULL);

    // pthread_create(&thrds[0], NULL, &b2, NULL);
    // pthread_join(thrds[0], NULL);

    //------------ Po dwa ---------------------
    // pthread_create(&thrds[0], NULL, &a1, NULL);
    // pthread_create(&thrds[1], NULL, &a2, NULL);
    // pthread_join(thrds[0], NULL);
    // pthread_join(thrds[1], NULL);

    // pthread_create(&thrds[0], NULL, &b1, NULL);
    // pthread_create(&thrds[1], NULL, &b2, NULL);
    // pthread_join(thrds[0], NULL);
    // pthread_join(thrds[1], NULL);

    //----------- Wszystko -------------------

    // pthread_create(&thrds[0], NULL, &a1, NULL);
    // pthread_create(&thrds[1], NULL, &a2, NULL);
    // pthread_create(&thrds[2], NULL, &b1, NULL);
    // pthread_create(&thrds[3], NULL, &b2, NULL);
    // pthread_join(thrds[0], NULL);
    // pthread_join(thrds[1], NULL);
    // pthread_join(thrds[2], NULL);
    // pthread_join(thrds[3], NULL);
}