struct Event
{
	int id;
	int x, y;
	int mapX, mapY;
	int image;
	EventScript *onTouchScript;
	EventScript *onExamineScript;
	std::vector<EventProperty *> properties;
};

