#include <ctime>
#include <vector>

#include "all.h"
#include "cmdline.h"

using namespace cmdline;

parser Cmdline_Parser(int argc, char * argv[])
{
    parser option;
    option.add <string> ("filename", 'f', "Original Circuit file", true);
    option.add <int> ("iteration", 'i', "Iteration times", false, 1);
    option.add <double> ("ratio", 'r', "Annealing rate", false, 0.99);
    option.add <bool> ("random", 'm', "Whether random order", false, false);
    option.add <string> ("operation", 'o',"Operation sequence (\"rw;rf;rs;b...\")", false, "rw");

    option.parse_check(argc, argv);
    return option;
}

vector<int> split(string str,string pattern)
{
  string::size_type pos;
  vector<int> result;
  str+=pattern;//扩展字符串以方便操作
  int size=str.size();

  for(int i=0; i<size; i++)
  {
    pos=str.find(pattern,i);
    if(pos<size)
    {
      string s=str.substr(i,pos-i);
      if (s == "b")
        result.push_back(0);
      else if (s == "rw")
        result.push_back(1);
      else if (s == "rf")
        result.push_back(2);
      else if (s == "rs")
        result.push_back(3);
      else
        cout << "One unknown command; please input \"rw/rf/rs/b\"" << endl;
    }
      i=pos+pattern.size()-1;
  }
  return result;
}



int main(int argc, char * argv[])
{

    // command line parser
    parser option = Cmdline_Parser(argc, argv);
    string filename = option.get <string> ("filename");
    int n = option.get <int> ("iteration");
    double ratio = option.get <double> ("ratio");
    bool random = option.get <bool> ("random");
    string operation = option.get<string> ("operation");

    // specify the operation order
    vector<int> operation_Sequence = split(operation,";");

    void (* SA_Operation[4])(double*, double, WHY_Man*,bool); // 定义一个长度为4的函数指针数组
    SA_Operation[0] = SA_Balance;
    SA_Operation[1] = SA_Rewrite;
    SA_Operation[2] = SA_Refactor;
    SA_Operation[3] = SA_Resub;


    double temperature = 1;

    WHY_Man * pMan = WHY_Start();
    char* Filename = (char*) filename.c_str();

    WHY_ReadBlif ( pMan, Filename );

    WHY_PrintStats ( pMan );
//
//    cout << endl;

    for (int i = 0;i<n;i++) {
        for (int j = 0; j < operation_Sequence.size();j++){
            // SA_Refactor ( &temperature,ratio,pMan,random );
            // SA_Rewrite ( &temperature,ratio,pMan,random );
            // Abc_NtkResubstitute( pMan->pNtk, 8, 1, 0, 1, 0, 0 );
            SA_Operation[operation_Sequence[j]] (&temperature,ratio,pMan,random);

        }
    }


    WHY_PrintStats ( pMan );

    // WHY_WriteBlif ( pMan, Filename );
    cout << endl;
    WHY_Stop( pMan );
    return 0;
}
