#include <ctime>
#include "refactor.h"
// #include "resub.h"
#include "cmdline.h"

using namespace cmdline;

parser Cmdline_Parser(int argc, char * argv[])
{
    parser option;
    option.add <string> ("filename", 'f', "Original Circuit file", true);
    option.add <int> ("iteration", 'i', "Iteration times", false, 1);
    option.add <double> ("ratio", 'r', "Annealing rate", false, 0.99);
    option.add <bool> ("random", 'm', "Whether random order", false, false);

    option.parse_check(argc, argv);
    return option;
}


int main(int argc, char * argv[])
{

    // command line parser
    parser option = Cmdline_Parser(argc, argv);
    string filename = option.get <string> ("filename");
    int n = option.get <int> ("iteration");
    double ratio = option.get <double> ("ratio");
    bool random = option.get <bool> ("random");
    double temperature = 1;

    WHY_Man * pMan = WHY_Start();
    char* Filename = (char*) filename.c_str();

    WHY_ReadBlif ( pMan, Filename );
    WHY_PrintStats ( pMan );
    cout << endl;



    for (int i = 0;i<n;i++)
        SA_Refactor ( &temperature,ratio,pMan,random );
        // Abc_NtkResubstitute( pMan->pNtk, 8, 1, 0, 1, 0, 0);


    WHY_PrintStats ( pMan );

    // WHY_WriteBlif ( pMan, Filename );
    cout << endl;
    WHY_Stop( pMan );
    return 0;
}
