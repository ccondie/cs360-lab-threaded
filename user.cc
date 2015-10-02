#include "user.h"

User::User(string name_t)
{
	name = name_t;
}

User::~User()
{

}

string
User::list()
{
	stringstream ss;
	ss << "list " << subject.size() << endl;

	for(int i = 1; i <= subject.size(); i++)
	{
		ss << i << " " << subject.at(i - 1) << "\n";
	}

	return ss.str();
}

string
User::read(int index)
{
	stringstream ss;

	if(index > subject.size())
	{
		//index out of bounds
		return "error index out of bounds";
	}
	else
	{
		ss << "message " << subject.at(index - 1) << " " << message.at(index - 1).length() << "\n" << message.at(index - 1) << "\n";
	}

	return ss.str();

}

