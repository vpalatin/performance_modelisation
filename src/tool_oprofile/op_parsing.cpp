#include <iostream>
#include <fstream>
#include <string>
#include <iostream>
#include <cassert>
#include <vector>
#include <iterator>
#include <unordered_map>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <limits>

using namespace std;

typedef unsigned long long ui64;
typedef unsigned long      ui32;

ui64 stoullhexa(string str)
{
        ui64 add=0;
        for(int i=0;;i++){
                char c=str[i];
                if((c>='0')and(c<='9')){add*=16;add+=(c-'0');continue;}
                if((c>='a')and(c<='f')){add*=16;add+=(c-'a'+10);continue;}
                if((c>='A')and(c<='F')){add*=16;add+=(c-'A'+10);continue;}
		break;
        }
        return(add);
}

const vector<string> split(const string& s, const char& c)
{
	string buff{""};
	vector<string> v;

	for(auto n:s)
	{
		if(n != c) buff+=n; else
		if(n == c && buff != "") { v.push_back(buff); buff = ""; }
	}
	if(buff != "") v.push_back(buff);
	return v;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   CLASS TO MANAGE OBJECT FILE created with "objdump -d binary"
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class objdump_line
{
public:
	objdump_line(string);
	string getstr(){return str;};
	ui64 get_address();
	string str;
	ui64 ctr1;
	ui64 ctr2;
	ui64 address;
	int type;  // 1=func : 2=inst
	int len_str;
	static int line_ctr;
private:
	int cur_line;
	int line_fname;
	int target; // branch target 0:not target 1=target
	static int last_func;
};

int objdump_line::line_ctr=0;
int objdump_line::last_func=0;

ui64 objdump_line::get_address()
{
	ui64 add=0;
	for(int i=2;;i++){
		char c=str[i];
		if(c==':')break;
		if(c==0)break; // should not occur
		if((c>='0')and(c<='9')){add*=16;add+=(c-'0');}
		if((c>='a')and(c<='f')){add*=16;add+=(c-'a'+10);}
		if((c>='A')and(c<='F')){add*=16;add+=(c-'A'+10);}
	}
	return(add);
}



// OBJECT FILE lines
vector<objdump_line> objdump_file;

// map address of instructions of OBJECT FILE to line inside objdump_file
// used for branch instruction to recover the line of the branch
std::unordered_map<ui64,int> objdump_address;

objdump_line::objdump_line(string line)
{
	ctr1=0;
	ctr2=0;
	target=0;
	line_ctr++;
	string fname;
	str=line;
	len_str=line.length();
	int type=0; // nothing usefull
	int inf=-1; // char poz of first '<'
	int sup=-1; // char poz of first '>'
	if(len_str>0){
		if(line[0]=='0'){
			// this line is a function name
			type=1;
			for (int i=0;i<len_str;i++){
				if(line[i]=='<')inf=i;
				if(line[i]=='>')sup=i;
			}
			if(inf==17){ // txt name
				fname=line.substr(inf+1,sup-inf-1);
				type=1; // function name
				last_func=line_ctr;
				//cout << "_1_ " << " " << inf << " " << sup << " " << fname << endl;
			}
			return;
		}
		if(line[0]==' '){
			// this line is an instruction assembly
			type=2; // instruction
			address=get_address();
			line_fname=last_func;
			objdump_address[address]=line_ctr;
			if(len_str>=32){
				string instr=line.substr(32,999);
				vector<string> v{split(instr,' ')};
				if(instr[0]=='j')
				if(v[0].compare("jmpq")!=0){
					ui64 add=stoullhexa(v[1]);
					int in_objdump=objdump_address[add];
					objdump_file[in_objdump].target=1;
					//cout << v[0] << " " << v[1] << endl;
				}
			}
			//cout << "_2_ " << std::dec << len_str << " " << line_ctr << " " << last_func << " " <<
				          //line << " " << address << " " <<  std::hex << address << endl;
			return;
		}
	}
}

void read_object_file(const char* fname)
{
    	ifstream fobj (fname);
	assert(  fobj.is_open());
	while(1){
		string line;
		getline(fobj, line);
    		if     (fobj.eof()) break;
		assert(!fobj.fail());
		assert(!fobj.bad());
		objdump_file.push_back(line);
		// all lines dumped in objdump_file from [0-line_ctr-1]
		//cout << "_7_ " << std::dec << objdump_line::line_ctr << objdump_file[objdump_line::line_ctr-1].str << endl;
	}
	fobj.close();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   CLASS TO MANAGE oprofile files captured with oprofile_line
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class oprofile_line
{
public:
	oprofile_line(string);
	string getstr(){return str;};
	void get_address_and_ctrs();
	string str;
	int type;  // 1=func : 2=inst
private:
	ui64 ctr1;
	ui64 ctr2;
	ui64 address;
	int cur_line;
	int len_str;
	int line_fname;
	static int line_ctr;
	static int last_func;
};

int oprofile_line::line_ctr=0;
int oprofile_line::last_func=0;

void oprofile_line::get_address_and_ctrs()
{
	ui64 add=0;
	for(int i=2;;i++){
		char c=str[i];
		if(c==' ')break;
		if(c==0)break; // should not occur
		if((c>='0')and(c<='9')){add*=16;add+=(c-'0');}
		if((c>='a')and(c<='f')){add*=16;add+=(c-'a'+10);}
		if((c>='A')and(c<='F')){add*=16;add+=(c-'A'+10);}
	}
	address=add;
	vector<string> v{split(str, ' ')};
	ctr1=stol(v[1]);
	ctr2=stol(v[3]);
	int in_objdump=objdump_address[add]-1;
	//cout << "_4_ ";for(auto n:v) cout << n << " "; cout << ctr1 << " " << ctr2 << " " << in_objdump << " "  << endl;
	if(in_objdump>0){
		objdump_file[in_objdump].ctr1=ctr1;
		objdump_file[in_objdump].ctr2=ctr2;
		//cout << "_3_ ";for(auto n:v) cout << n << " "; cout << ctr1 << " " << ctr2 << " " << in_objdump << " " << objdump_file[in_objdump].str << endl;
	}
}



// OBJECT FILE lines
vector<oprofile_line> oprofile_file;

// map address of instructions of OBJECT FILE to line inside oprofile_file
// used for branch instruction to recover the line of the branch
std::unordered_map<ui64,int> oprofile_address;

oprofile_line::oprofile_line(string line)
{
	ctr1=0;
	ctr2=0;
	line_ctr++;
	string fname;
	str=line;
	len_str=line.length();
	type=0; // nothing usefull
	if(len_str>0){ // function name
		if(line[0]=='0'){
			last_func=line_ctr;
			type=1;
			//cout << "_1_ " << line << endl;
			return;
		}
		if(line[0]==' '){ // instruction
			get_address_and_ctrs();
			line_fname=last_func;
			oprofile_address[address]=line_ctr;
			//cout << "_2_ " << std::dec << len_str << " " << line_ctr << " " << last_func << " "
				       //<< line << " " << address << " " <<  std::hex << address << " " << std::dec
				       //<< ctr1 << " " << ctr2 << endl;
			type=2;
			return;
		}
	}
}
void read_oprofile_file(const char* fname)
{
    	ifstream fop2 (fname);
	assert(  fop2.is_open());
	while(1){
		string line;
		getline(fop2, line);
    		if     (fop2.eof()) break;
		assert(!fop2.fail());
		assert(!fop2.bad());
		oprofile_file.push_back(line);
	}
	fop2.close();
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void results()
{
	// we have all data now ; we will reconstruct the profile in same order
	// we have to present the hot spots in same order as op2
	cout << "================================================================================================================" << endl;
	cout << "================================================================================================================" << endl;
	for(int line=0;line<oprofile_file.size();line++){
		if( oprofile_file[line].type == 0)  cout << "_0_ " << oprofile_file[line].str << endl;
		if( oprofile_file[line].type == 1){
			vector<string> v{split(oprofile_file[line].str, ' ')};
			double cycles=stof(v[2]);
			if(cycles>0.01)cout << "_1_ " << oprofile_file[line].str << endl;
		}
	}
	cout << "================================================================================================================" << endl;
	cout << "================================================================================================================" << endl;
	for(int line=0;line<oprofile_file.size();line++){
		if( oprofile_file[line].type == 1){
			vector<string> v{split(oprofile_file[line].str, ' ')};
			double cycles=stof(v[2]);
			ui64 add=stoullhexa(v[0]);
			// we dump the disassembly of all functions consuming more than 0.1% of total
			if(cycles>0.1){
				// we search the line in objdump
				int in_objdump=objdump_address[add];
				cout << "_2_ " << add << " " << in_objdump << " " << v[0] << " "  << oprofile_file[line].str << endl;
				cout << "----------------------------------------------------------------------------------------------------------------" << endl;
		      	      //cout << "_5_        87670        28368        28366 |        23005           41      400854:     c4 c2 a5 a8 cb          vfmadd213pd %ymm11,%ymm11,%ymm1
				cout << "           SUM*4        SUM*3        SUM*2          CYCLES       INSTS      ADDRESS     code HEXA               disassembly"  << endl;
				cout << "----------------------------------------------------------------------------------------------------------------" << endl;
				if(in_objdump>0)
				for(int li=in_objdump-1;;li++){
					int   type=objdump_file[li].type;
					ui64  ctr1=objdump_file[li].ctr1;
					ui64  ctr2=objdump_file[li].ctr2;
					string str=objdump_file[li].str;
					int    len=objdump_file[li].len_str;
					ui64 myadd=objdump_file[li].address;
					ui64 sum2=ctr1;
					     sum2+=objdump_file[li+1].ctr1;
					ui64 sum3=sum2;
					     sum3+=objdump_file[li+2].ctr1;
					ui64 sum4=sum3;
					     sum4+=objdump_file[li+3].ctr1;
					if(len==0)break;
					if(ctr1>0)
					if(ctr2>0)
						cout <<  "_5_ " << std::setw(12) << sum4 << " "
						       		<< std::setw(12) << sum3 << " "
						       		<< std::setw(12) << sum2 << " | "
						       		<< std::setw(12) << ctr1 << " "
						       		<< std::setw(12) << ctr2 << "    " << str << endl;
					if(len>=32){ // we try to detect a loop
						string instr=str.substr(32,999);
						vector<string> v{split(instr,' ')};
						if(instr[0]=='j')
						if(v[0].compare("jmpq")!=0){
							ui64 add=stoullhexa(v[1]);
							ui64 diff=myadd-add;
							if(diff<100000ull){ // backward loop
								// we will sum the cycles and instructions of the loop
								ui64 sumcy=0;
								ui64 sumin=0;
								int  count=0;
								for(int li2=li;;li2--){
									ui64  ctr1=objdump_file[li2].ctr1;
									ui64  ctr2=objdump_file[li2].ctr2;
									sumcy+=ctr1;
									sumin+=ctr2;
									count++;
									ui64  addt=objdump_file[li2].address;
									if(addt==add)break;
								}
								double IPC=double(sumin)/double(sumcy);
								double cyL=double(count)/IPC;
								cout << "----------------------------------------------------------------------------------------------------------------" << endl;
								cout << "_7_ LOOP from " << std::hex << myadd << " to " << std::hex << add << " size= " << std::dec << diff  << " sum(cycles)= "
								               << sumcy << " sum(inst)= " << sumin << " #inst= " << count << " IPC= " << IPC << " cycles/LOOP= " << cyL  << endl;
								cout << "----------------------------------------------------------------------------------------------------------------" << endl;
							}
						}
					}
				}
			}
		}
	}
	cout << "================================================================================================================" << endl;
	cout << "================================================================================================================" << endl;

}

int main(int argc, char* argv[]) {
    	string line;
    	if(argc != 3) {
        	cerr << "command Object_file oprofile_file" << endl;
        	return 1;
    	}
	read_object_file  (argv[1]);
	read_oprofile_file(argv[2]);
	results();
}


