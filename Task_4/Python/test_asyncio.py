import asyncio
import time

async def func1():
    print(1)


async def func2():
    await asyncio.sleep(1)
    print(2)


async def func3():
    print(3)

async def main():
    task1 = asyncio.create_task(func1())
    task2 = asyncio.create_task(func2())
    task3 = asyncio.create_task(func3())
    tasks = [task1,task2,task3]

    await asyncio.gather(*tasks)
    # await asyncio.gather(task1,task2,task3)



asyncio.run(main())
