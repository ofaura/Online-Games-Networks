#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <mutex>

long long int myGlobal = 0;

void function()
{
	for (unsigned int i = 0; i < 1000000; ++i)
	{
		myGlobal++;
	}
}

int main()
{
	std::thread t[] =
	{
		std::thread(function),
		std::thread(function)
	};
		
	// Wait untill this thread finished
	t[0].join();
	t[1].join();

	printf("myGlobal %lld\n", myGlobal);

	system("pause");
	return 0;
}

while (true)
{
	{
		lock(mutex);

		if (taskList.size() > 0)
		{

		}
		else
		{
			condition_variable.wait(lock);
		}
	}
}