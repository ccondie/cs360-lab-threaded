#include "message.h"

Message::Message()
{
	
}

Message::~Message()
{
	
}

string
Message::toString()
{
	stringstream ss;

	ss << "\tMessage.toString()" << endl;
	ss << "\t\tcommand:" << command << endl;
	ss << "\t\tparam[0]:" << param[0] << endl;
	ss << "\t\tparam[1]:" << param[1] << endl;
	ss << "\t\tvalue:" << value << endl;
	ss << "\t\tneeded:" << needed << endl;
	ss << "\t\tmessage:" << message << endl;

	return ss.str();
}