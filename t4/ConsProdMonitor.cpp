#include <string>
#include <vector>
#include <iostream>

#define SLEEP usleep(1000000 + rand() % 1000000)
#define PRINT(x) std::cout << x << std::endl

#include <pthread.h>
#include <unistd.h>

#include "monitor.h"

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
            if (buffer[i] % 2 == 0)
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
        if (buffer.empty())
        {
            return -1;
        }
        return buffer[0];
    }
};

class ConsProdMonitor : Monitor
{

    Buffer buffer;
    Condition prodEvenCond, prodOddCond, consEvenCond, consOddCond;
    unsigned int numOfProdEvenWaiting = 0;
    unsigned int numOfProdOddWaiting = 0;
    unsigned int numOfConsEvenWaiting = 0;
    unsigned int numOfConsOddWaiting = 0;

public:
    void putEven(unsigned int num);
    void putOdd(unsigned int num);

    void getEven();
    void getOdd();

    bool canProdEven() { return (buffer.countEven() < 10); };

    bool canProdOdd() { return buffer.countEven() > buffer.countOdd(); };

    bool canConsEven() { return (buffer.count() >= 3 && buffer.peekFirst() != -1 && buffer.peekFirst() % 2 == 0); };

    bool canConsOdd() { return (buffer.count() >= 7 && buffer.peekFirst() != -1 && buffer.peekFirst() % 2 != 0); };
};

void ConsProdMonitor::putEven(unsigned int num)
{
    enter();
    
    if (!canProdEven())
    {
        numOfProdEvenWaiting++;
        leave();
        wait(prodEvenCond);
        numOfProdEvenWaiting--;
    }
    
    buffer.put(num);

    std::string s = "putEven: Dodano " + std::to_string(num);
    PRINT(s);

    std::cout << "{ ";
    for (int n : buffer.Buff()) {
        std::cout << n << ", ";
    }
    std::cout << "}; \n";
    
    if (canProdOdd() && numOfProdOddWaiting > 0)
    {
        signal(prodOddCond);
    }
    else if (canConsEven() && numOfConsEvenWaiting > 0)
    {
        signal(consEvenCond);
    }
    else if (canConsOdd() && numOfConsOddWaiting > 0)
    {
        signal(consOddCond);
    }
    else
    {
        leave();
    }
}

void ConsProdMonitor::putOdd(unsigned int num)
{
    enter();
    
    if (!canProdOdd())
    {
        numOfProdOddWaiting++;
        leave();
        wait(prodOddCond);
        numOfProdOddWaiting--;
    }
    
    buffer.put(num);

    std::string s = "putOdd: Dodano " + std::to_string(num);
    PRINT(s);

    std::cout << "{ ";
    for (int n : buffer.Buff()) {
        std::cout << n << ", ";
    }
    std::cout << "}; \n";

    if (canProdEven() && numOfProdEvenWaiting > 0)
    {
        signal(prodEvenCond);
    }
    else if (canConsEven() && numOfConsEvenWaiting > 0)
    {
        signal(consEvenCond);
    }
    else if (canConsOdd() && numOfConsOddWaiting > 0)
    {
        signal(consOddCond);
    }
    else
    {
        leave();
    }
}

void ConsProdMonitor::getEven()
{
    enter();
    
    if (!canConsEven())
    {
        numOfConsEvenWaiting++;
        leave();
        wait(consEvenCond);
        numOfConsEvenWaiting--;
    }
    
    auto num = buffer.get();

    std::string s = "getEven: Zabrano " + std::to_string(num);
    PRINT(s);

    std::cout << "{ ";
    for (int n : buffer.Buff()) {
        std::cout << n << ", ";
    }
    std::cout << "}; \n";
    
    if (canProdEven() && numOfProdEvenWaiting > 0)
    {
        signal(prodEvenCond);
    }
    else if (canProdOdd() && numOfProdOddWaiting > 0)
    {
        signal(prodOddCond);
    }
    else if (canConsOdd() && numOfConsOddWaiting > 0)
    {
        signal(consOddCond);
    }
    else
    {
        leave();
    }
}

void ConsProdMonitor::getOdd()
{
    enter();
    
    if (!canConsOdd())
    {
        numOfConsOddWaiting++;
        leave();
        wait(consOddCond);
        numOfConsOddWaiting--;
    }
    
    auto num = buffer.get();

    std::string s = "getOdd: Zabrano " + std::to_string(num);
    PRINT(s);

    std::cout << "{ ";
    for (int n : buffer.Buff()) {
        std::cout << n << ", ";
    }
    std::cout << "}; \n";
    
    if (canProdEven() && numOfProdEvenWaiting > 0)
    {
        signal(prodEvenCond);
    }
    else if (canProdOdd() && numOfProdOddWaiting > 0)
    {
        signal(prodOddCond);
    }
    else if (canConsEven() && numOfConsEvenWaiting > 0)
    {
        signal(consEvenCond);
    }
    else
    {
        leave();
    }
}

ConsProdMonitor monitor;

void* a1(void* arg)
{
    for(unsigned int i = 0;; i = (i+2) % 50)
    {
        monitor.putEven(i);
        SLEEP;
    }
}

void* a2(void* arg)
{
    for(unsigned int i = 1;; i = (i+2) % 50)
    {
        monitor.putOdd(i);
        SLEEP;
    }
}

void* b1(void* arg)
{
    while(1)
    {
        monitor.getEven();
        SLEEP;
    }
}

void* b2(void* arg)
{
    while(1)
    {
        monitor.getOdd();
        SLEEP;
    }
}

int main(int argc, char* argv[])
{

    srand(time(NULL));
    pthread_t thrds[4];

    if (argc == 1)
    {
        std::cout << "Wybierz scenariusz jako agrument (1-7)" << std::endl;
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
}