/*
 * @author: shenke
 * @date: 2022/3/14
 * @project: HuaWei_CodeCraft_2022
 * @desp:
 */

#include "Handler.h"

int main(int argc, char *argv[]) {

    Handler handler;
    handler.Read();
    handler.Handle();
    handler.Output();

    exit(0);
}
