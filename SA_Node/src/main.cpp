#include <ctime>
#include <vector>

#include "all.h"
#include "cmdline.h"

using namespace cmdline;

parser Cmdline_Parser(int argc, char * argv[])
{
    parser option;
    option.add <string> ("inputFile", 'f', "Original Circuit file", true);
    // option.add <int> ("iteration", 'i', "Iteration times", false, 1);
    // option.add <double> ("ratio", 'r', "Annealing rate", false, 0.99);
    option.add <double> ("temperature", 't', "Whether Random order", false, 1);
    option.add <string> ("operation", 'r',"Rewriting Operation( b / rw / rs/ rf )", false, "rw");
    option.add <string> ("outputFile", 'o', "Output Circuit file", true);

    option.parse_check(argc, argv);
    return option;
}

int operationMap(string str)
{

    int result;
    if (str == "b")
        return 0;
      else if (str == "rw")
        return 1;
      else if (str == "rf")
        return 2;
      else if (str == "rs")
        return 3;
      else
        cout << "One unknown command; please input rw/rf/rs/b" << endl;
        return -1;
}



int main(int argc, char * argv[])
{

    // command line parser
    parser option = Cmdline_Parser(argc, argv);
    string filename = option.get <string> ("inputFile");
    // int n = option.get <int> ("iteration");
    // double ratio = option.get <double> ("ratio");
    // bool random = option.get <bool> ("random");
    double temp = option.get <double> ("temperature");
    string operation = option.get<string> ("operation");
    string filename2 = option.get <string> ("outputFile");

    // specify the operation order
    int operationNum = operationMap(operation);

    void (* SA_Operation[4])(double , WHY_Man*); // 定义一个长度为4的函数指针数组
    SA_Operation[0] = SA_Balance;
    SA_Operation[1] = SA_Rewrite;
    SA_Operation[2] = SA_Refactor;
    SA_Operation[3] = SA_Resub;


    // double temperature = 1;

    WHY_Man * pMan = WHY_Start();
    char* Filename = (char*) filename.c_str();
    char* Filename2 = (char*) filename2.c_str();


    WHY_ReadBlif ( pMan, Filename );

    WHY_PrintStats ( pMan );
//
//    cout << endl;

    SA_Operation[operationNum] (temp,pMan);



    WHY_PrintStats ( pMan );

    WHY_WriteBlif ( pMan, Filename2 );
    cout << endl;
    WHY_Stop( pMan );
    return 0;
}
