#include <stdio.h>
#include <stdarg.h>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>
#include <iostream>

#include "std_ext.h"
#include "output.h"

int CRC32(const void *buffer, int size, int crcstart) {
	static unsigned randBits[] = {
		0x00000001, 0x2C11F801, 0xDFD8F60E, 0x6C8FA2B7,
		0xB573754C, 0x1522DCDD, 0x21615D3A, 0xE1B307F3,
		0x12AFA158, 0x53D18179, 0x70950126, 0x941702EF,
		0x756FE824, 0x694801D5, 0x36DF4DD2, 0x63D80FAB,
		0xB8AE95B0, 0x902439F1, 0x090C6F3E, 0x2B7C6A27,
		0x8344B5FC, 0x67D3C5CD, 0x22F5516A, 0x2FB00E63,
		0xFC761508, 0x87B00169, 0x27EBA056, 0x8CC0B85F,
		0xE33D3ED4, 0x95DA08C5, 0x13E5C802, 0x9DD9E41B,
		0xD4577F60, 0x3DD6B7E1, 0x096AF46E, 0x1A00CD97,
		0x4B10E2AC, 0x22EAAABD, 0x683F119A, 0x62D070D3,
		0xA8D034B8, 0xAA363D59, 0x58CECB86, 0x40F589CF,
		0x4F630184, 0x38918BB5, 0xB85B8E32, 0x0A6A948B,
		0x9A099510, 0x402871D1, 0x11E7859E, 0xEE73CD07,
		0x4142FB5C, 0x39D68BAD, 0x0FE19DCA, 0xE35B2F43,
		0x75590068, 0x66433549, 0x929182B6, 0x71EC773F,
		0xBBAC3034, 0xF2BD8AA5, 0x5743A062, 0x5AB120FB,
		0x5ABFD6C0, 0xDDD867C1, 0xDC3522CE, 0xD0EC6877,
		0xE106000C, 0xB7C6689D, 0xED3FF5FA, 0xC75749B3,
		0x126B7818, 0x1A75E939, 0x0546C5E6, 0x8A9C80AF,
		0x48A3CAE4, 0x756D0595, 0x7060FE92, 0xA594896B,
		0x12354470, 0x896599B1, 0xDAC6CBFE, 0xCB419FE7,
		0x9C44F0BC, 0xAFA9418D, 0xB87D1A2A, 0x428BC023,
		0x33229BC8, 0xC92D5929, 0xB1C19516, 0x0FBCA61F,
		0xE594D194, 0x716EFC85, 0x0036A8C2, 0xD7BBCDDB,
		0x16E4DE20, 0xD10F07A1, 0x68CF812E, 0x390A7357,
		0x8BAACD6C, 0x2C2E167D, 0x3E7C0A5A, 0x167F9293,
		0x3D596B78, 0x08888519, 0x9994F046, 0x0FC3E78F,
		0x008A4444, 0x87526F75, 0xB0079EF2, 0x238DEE4B,
		0xCA09A3D0, 0x4ED3B191, 0xFA42425E, 0x379DE2C7,
		0x1EA2961C, 0x1FC3E76D, 0x90DFC68A, 0x0279C103,
		0xF9AAE728, 0xF2666D09, 0xEF13D776, 0x92E944FF,
		0x364F22F4, 0x37665E65, 0x05D6E122, 0x7131EABB,
		0x479E9580, 0x98729781, 0x4BD20F8E, 0x1612EE37,
		0xCB574ACC, 0x5499B45D, 0x360B4EBA, 0x33814B73,
		0x43720ED8, 0x146610F9, 0x45514AA6, 0x0B23BE6F,
		0x026E6DA4, 0xD1B9C955, 0x94676F52, 0xCE8EC32B,
		0x165EB330, 0x2F6AB971, 0x92F1E8BE, 0xC54095A7,
		0xBEB3EB7C, 0x5C9E7D4D, 0x5921A2EA, 0xB45D31E3,
		0xB8C9E288, 0x5FE670E9, 0xC02049D6, 0xC42A53DF,
		0x6F332454, 0x661BB045, 0x2B3C4982, 0xDF4B779B,
		0xD7C4FCE0, 0x70FB1761, 0xADD4CDEE, 0x47BDD917,
		0x8C63782C, 0x8181423D, 0xFA05C31A, 0xDD947453,
		0x6A8D6238, 0x1A068CD9, 0x4413D506, 0x5374054F,
		0xC5A84704, 0xB41B1335, 0x06986FB2, 0x4CCF080B,
		0xF80C7290, 0x8622B151, 0x536DBF1E, 0x21E1B887,
		0xDED0F0DC, 0xB4B1032D, 0x1D5AAF4A, 0xC56E12C3,
		0x8C578DE8, 0xCBA564C9, 0xA67EEC36, 0x0837D2BF,
		0x3D98D5B4, 0x1B06F225, 0xFF7EE1E2, 0x3640747B,
		0x5E301440, 0x53A08741, 0x436FBC4E, 0xC9C333F7,
		0x2727558C, 0x7F5CC01D, 0xFC83677A, 0xAFF10D33,
		0x24836598, 0x3161F8B9, 0xDD748F66, 0x5B6CBC2F,
		0xAD8FD064, 0x89EE4D15, 0xBBB2A012, 0xA086BCEB,
		0x1BEAE1F0, 0x69F39931, 0x764DC57E, 0x17394B67,
		0x4D51A63C, 0xF273790D, 0x35A2EBAA, 0x7EE463A3,
		0xBC2BE948, 0x2B9B48A9, 0x2FC7BE96, 0x5FC9C19F,
		0x3AD83714, 0x6FA02405, 0xDDB6AA42, 0xE648E15B,
		0x1DB7DBA0, 0xF55AE721, 0x4D3ADAAE, 0xB3DAFED7,
		0x5FFAE2EC, 0x96A42DFD, 0xFB9C3BDA, 0x21CF1613,
		0x0F2C18F8, 0xAE705499, 0x650B79C6, 0x31C5E30F,
		0x097D09C4, 0xAAAB76F5, 0x34CE0072, 0x27EDE1CB,
		0xDAD20150, 0xADD57111, 0xC229FBDE, 0x8AFF4E47,
		0x448E0B9C, 0x5C5DDEED, 0x4612580A, 0x05F82483,
		0xBC1EF4A8, 0xB1C01C89, 0xF592C0F6, 0x6798207F,
		0xEC494874, 0x795F45E5, 0xECFBA2A2, 0xBB9CBE3B,
		0xF567104f, 0x47289407, 0x25683fa6, 0x2fde5836,
	};


	int i;
	unsigned c;
	unsigned crc;
	unsigned val;
	unsigned char *ptr;

	crc = crcstart;
	ptr = (unsigned char*)buffer;
	for (i = 0; i < size; i ++)
	{
		c = ptr[i];
		val = randBits[((crc >> 24) ^ c) & 0xff];
		crc = (crc << 8) ^ val;
	}
	return crc;
}

namespace stdx {
int memicmp(const void* vs1,const void* vs2,size_t n) {
	size_t i;
	const char* s1=(const char*)vs1;
	const char* s2=(const char*)vs2;
	for(i=0;i<n;i++) {
		unsigned char u1=s1[i];
		unsigned char u2=s2[i];
		int U1=std::toupper(u1);
		int U2=std::toupper(u2);
		int diff=(UCHAR_MAX <= INT_MAX ? U1 - U2 : U1 < U2 ? -1 : U2 < U1);
		if(diff)
			return diff;
	}
	return 0;
}

std::string url_decode(const std::string& url) {
	std::string s2;
	s2.reserve(url.size());
	for (size_t i=0;i<url.size();i++) {
		if(url[i]=='%') {
			if(!strncmp(&url[i],"%20",3))
				s2+=' ';
			if (!strncmp(&url[i],"%3A",3))
				s2+=':';
			else if(!strncmp(&url[i],"%2F",3))
				s2+='/';
			else if(!strncmp(&url[i],"%3F",3))
				s2+='?';
			else if(!strncmp(&url[i],"%3D",3))
				s2+='=';
			else if(!strncmp(&url[i],"%26",3))
				s2+='&';
			else if(!strncmp(&url[i],"%25",3))
				s2+='%';
			i+=2;
		}else
			s2+=url[i];
	}
	return s2;
}

std::string format_string(const char* format,...) {
	va_list v;
	va_start(v,format);
	char res[8192];
	va_list v2;
	va_copy(v2,v);
	int len=vsnprintf(NULL,0,format,v2);
	if(len<(int)sizeof(res)) {
		if(vsnprintf(res,sizeof(res),format,v)!=len)
			FATAL("format_string failed 0");
		va_end(v);
		va_end(v2);
		return std::string(res);
	}
	char* buf=new char[len+1];
	if(vsnprintf(buf,len,format,v)!=len)
		FATAL("format_string failed 1");
	va_end(v);
	va_end(v2);
	std::string out=buf;
	delete [] buf;
	return out;
}
void tolower(std::string* str) {
	std::transform(str->begin(),str->end(),str->begin(),[](unsigned char c){
		return std::tolower(c);
	});
}
std::string tolower(const std::string& str) {
	std::string str1=str;
	tolower(&str1);
	return str1;
}
bool starts_with(const std::string& value,const std::string& begining) {
	return value.rfind(begining,0)==0;
}

bool ends_with(const std::string& value,const std::string& ending) {
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

bool exists(const std::string& str,const std::vector<std::string>& strings) {
	if(std::find(strings.begin(),strings.end(),str)!=strings.end()) {
		return true;
	}
	return false;
}


int hash32(const std::string& str) {
	return CRC32(str.c_str(),(int)str.size(),0);
}


bool EndsWith(const std::string& value,const std::string& ending) {
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}
std::string FromStdArray(const std::vector<std::string>& lst) {
	int length=0;
	int elements=(int)lst.size();
	for(int i=0;i!=elements;i++) {
		length+=(int)lst[i].size()+1;
	}
	std::string merged;
	merged.reserve(length);
	for(int i=0;i!=elements;i++) {
		merged+=lst[i];
		if(i!=elements-1)
			merged+=",";
	}

	return merged;
}

std::string spaces(int count) {
	return std::string(count,' ');
}

std::string format_vector(const std::vector<int>& s,const char* format,const char* seperator) {
	std::string out;
	for(auto& v:s) {
		std::string ss=stdx::format_string(format,v);
		if(out.size())
			out+=",";
		out+=ss;
	}
	return out;
}
std::string format_vector(const std::vector<uint8_t>& s,const char* format,const char* seperator) {
	std::string out;
	for(auto& v:s) {
		std::string ss=stdx::format_string(format,v);
		if(out.size())
			out+=",";
		out+=ss;
	}
	return out;
}
std::string format_vector(const std::vector<float>& s,const char* format,const char* seperator) {
	std::string out;
	for(auto& v:s) {
		std::string ss=stdx::format_string(format,v);
		if(out.size())
			out+=",";
		out+=ss;
	}
	return out;
}

std::vector<std::string> Split(const std::string& s,char seperator) {
	std::vector<std::string> output;
	std::string::size_type prev_pos=0, pos=0;
	while((pos=s.find(seperator, pos)) != std::string::npos) {
		std::string substring( s.substr(prev_pos, pos-prev_pos) );
		output.push_back(substring);
		prev_pos=++pos;
	}
	output.push_back(s.substr(prev_pos, pos-prev_pos)); // Last word
	return output;
}

void ReplaceAll(std::string* str, const std::string& from, const std::string& to) {
    if(from.empty())
        return;
    size_t start_pos=0;
    while((start_pos=str->find(from, start_pos)) != std::string::npos) {
        str->replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

std::string to_string(const M44& m) {
	std::string s=stdx::format_string("%g,%g,%g,%g %g,%g,%g,%g %g,%g,%g,%g %g,%g,%g,%g",m[0],m[1],m[2],m[3],m[4],m[5],m[6],m[7],m[8],m[9],m[10],m[11],m[12],m[13],m[14],m[15]);
	return s;
}

};
