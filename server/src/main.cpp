#include <iostream>
#include "Conn.h"
#include "config.h"
#include "Context.h"

int main()
{
    Conn conn(PORT);
    conn.wait([](Context &context){
        while (context.isAvail())
            std::cout << context.next() << std::endl;
    });
    return 0;
}

