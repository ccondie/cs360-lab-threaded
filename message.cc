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

	ss << "Message.toString()" << endl;
	ss << "\tcommand:" << command << endl;
	ss << "\tparam[0]:" << param[0] << endl;
	ss << "\tparam[1]:" << param[1] << endl;
	ss << "\tvalue:" << value << endl;
	ss << "\tneeded:" << needed << endl;
	ss << "\tcache:" << cache << endl;

	return ss.str();
}