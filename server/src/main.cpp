#include "Conn.h"
#include "config.h"
#include "Context.h"

int main()
{
    Conn conn(PORT);
    conn.wait([](Context &context){});
    return 0;
}

