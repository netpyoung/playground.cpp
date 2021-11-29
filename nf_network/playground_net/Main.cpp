#include "chat/App.hpp"

using namespace chat;

int main()
{
	return App{}.Run("0.0.0.0", 11501u);
}
