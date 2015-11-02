#include "Robot.hpp"
#include <sstream>
#include "Thread.hpp"
#include <boost/chrono.hpp>
#include <ctime>
#include "Logger.hpp"
#include "Goal.hpp"
#include "WayPoint.hpp"
#include "Wall.hpp"
#include "RobotWorld.hpp"
#include "Math.hpp"
#include "Shape2DUtils.hpp"
#include "CommunicationService.hpp"
#include "Client.hpp"
#include "Message.hpp"
#include "ConfigFile.hpp"

#include<regex>
#include<string>

#include <stdio.h>      /* printf, scanf, puts, NULL */
#include <stdlib.h>     /* srand, rand */
#include <time.h>

/**
 *
 */
Robot::Robot( const std::string& aName) :
				name( aName),
				size( DefaultSize),
				position( DefaultPosition),
				front( 0, 0),
				speed( 0.0),
				stop(true),
				communicating(false),
				original(true),
				robotId(1)
{
	attachSensor(std::shared_ptr< AbstractSensor >(new LaserDistanceSensor(this,150,50)),true);
	std::srand(time(NULL));
	robotId = (rand() % 899999999 + 100000000);
}
/**
 *
 */
Robot::Robot(	const std::string& aName,
				const Point& aPosition) :
				name( aName),
				size( DefaultSize),
				position( aPosition),
				front( 0, 0),
				speed( 0.0),
				stop(true),
				communicating(false),
				original(true),
				robotId(1)
{
	attachSensor(std::shared_ptr< AbstractSensor >(new LaserDistanceSensor(this,150,50)),true);
	std::srand(time(NULL));
	robotId = (rand() % 899999999 + 100000000);
}
/**
 *
 */
Robot::Robot(	const std::string& aName,
				const Point& aPosition,
				const bool& aOriginal) :
				name( aName),
				size( DefaultSize),
				position( aPosition),
				front( 0, 0),
				speed( 0.0),
				stop(true),
				communicating(false),
				original(aOriginal),
				robotId(1)
{
	attachSensor(std::shared_ptr< AbstractSensor >(new LaserDistanceSensor(this,150,50)),true);
	std::srand(time(NULL));
	robotId = (rand() % 899999999 + 100000000);
}
/**
 *
 */
Robot::Robot(	const std::string& aName,
				const Point& aPosition,
				const bool& aOriginal,
				const long long& aId) :
				name( aName),
				size( DefaultSize),
				position( aPosition),
				front( 0, 0),
				speed( 0.0),
				stop(true),
				communicating(false),
				original(aOriginal),
				robotId(aId)
{
	attachSensor(std::shared_ptr< AbstractSensor >(new LaserDistanceSensor(this,150,50)),true);
}
/**
 *
 */
Robot::~Robot()
{
	if(!stop)
	{
		stopActing();
	}
	if(communicating)
	{
		stopCommunicating();
	}
}
/**
 *
 */
void Robot::setName( const std::string& aName)
{
	name = aName;
	notifyObservers();
}
/**
 *
 */
Size Robot::getSize() const
{
	return size;
}
/**
 *
 */
void Robot::setSize(	const Size& aSize,
						bool aNotifyObservers /*= true*/)
{
	size = aSize;
	if (aNotifyObservers == true)
	{
		notifyObservers();
	}
}
/**
 *
 */
void Robot::setPosition(	const Point& aPosition,
							bool aNotifyObservers /*= true*/)
{
	position = aPosition;
	if (aNotifyObservers == true)
	{
		notifyObservers();
	}
}
/**
 *
 */
Vector Robot::getFront() const
{
	return front;
}
/**
 *
 */
void Robot::setFront(	const Vector& aVector,
						bool aNotifyObservers /*= true*/)
{
	front = aVector;
	if (aNotifyObservers == true)
	{
		notifyObservers();
	}
}
/**
 *
 */
float Robot::getSpeed() const
{
	return speed;
}
/**
 *
 */
void Robot::setSpeed( float aNewSpeed)
{
	speed = aNewSpeed;
}
/**
 *
 */
void Robot::startActing()
{
	std::thread newRobotThread( [this]
	{	stop = false;});
	//robotThread.swap( newRobotThread);
}
/**
 *
 */
void Robot::stopActing()
{
	//robotThread.interrupt();
	stop = true;
	robotThread.join();
	for (std::shared_ptr< AbstractSensor > sensor : sensors)
	{
		sensor->setOff();
	}
}
/**
 *
 */
void Robot::calculatePath()
{
	GoalPtr goal = RobotWorld::getRobotWorld().getGoal();
	calculateRoute(goal);
}
/**
 *
 */
void Robot::startMoving()
{
	GoalPtr goal = RobotWorld::getRobotWorld().getGoal();
	std::thread newRobotThread( [this,goal]
	{	drive(goal);});
}
/**
 *
 */
Region Robot::getRegion() const
{
	Point translatedPoints[] = { getFrontRight(), getFrontLeft(), getBackLeft(), getBackRight() };
	return Region( 4, translatedPoints);
}
/**
 *
 */
bool Robot::intersects( const Region& aRegion) const
{
	Region region = getRegion();
	region.Intersect( aRegion);
	return !region.IsEmpty();
}
/**
 *
 */
Point Robot::getFrontLeft() const
{
	// x and y are pointing to top left now
	int x = position.x - (size.x / 2);
	int y = position.y - (size.y / 2);

	Point originalFrontLeft( x, y);
	double angle = Shape2DUtils::getAngle( front) + 0.5 * PI;

	Point frontLeft( (originalFrontLeft.x - position.x) * std::cos( angle) - (originalFrontLeft.y - position.y) * std::sin( angle) + position.x, (originalFrontLeft.y - position.y) * std::cos( angle)
										+ (originalFrontLeft.x - position.x) * std::sin( angle) + position.y);

	return frontLeft;
}
/**
 *
 */
Point Robot::getFrontRight() const
{
	// x and y are pointing to top left now
	int x = position.x - (size.x / 2);
	int y = position.y - (size.y / 2);

	Point originalFrontRight( x + size.x, y);
	double angle = Shape2DUtils::getAngle( front) + 0.5 * PI;

	Point frontRight( (originalFrontRight.x - position.x) * std::cos( angle) - (originalFrontRight.y - position.y) * std::sin( angle) + position.x, (originalFrontRight.y - position.y)
					* std::cos( angle) + (originalFrontRight.x - position.x) * std::sin( angle) + position.y);

	return frontRight;
}
/**
 *
 */
Point Robot::getBackLeft() const
{
	// x and y are pointing to top left now
	int x = position.x - (size.x / 2);
	int y = position.y - (size.y / 2);

	Point originalBackLeft( x, y + size.y);

	double angle = Shape2DUtils::getAngle( front) + 0.5 * PI;

	Point backLeft( (originalBackLeft.x - position.x) * std::cos( angle) - (originalBackLeft.y - position.y) * std::sin( angle) + position.x, (originalBackLeft.y - position.y) * std::cos( angle)
									+ (originalBackLeft.x - position.x) * std::sin( angle) + position.y);

	return backLeft;

}
/**
 *
 */
Point Robot::getBackRight() const
{
	// x and y are pointing to top left now
	int x = position.x - (size.x / 2);
	int y = position.y - (size.y / 2);

	Point originalBackRight( x + size.x, y + size.y);

	double angle = Shape2DUtils::getAngle( front) + 0.5 * PI;

	Point backRight( (originalBackRight.x - position.x) * std::cos( angle) - (originalBackRight.y - position.y) * std::sin( angle) + position.x, (originalBackRight.y - position.y) * std::cos( angle)
										+ (originalBackRight.x - position.x) * std::sin( angle) + position.y);

	return backRight;
}
/**
 *
 */
void Robot::handleNotification()
{
	std::unique_lock<std::recursive_mutex> lock(robotMutex);

	static int update = 0;
	if ((++update % 200) == 0)
	{
//		std::cerr << __PRETTY_FUNCTION__ << std::endl;
		notifyObservers();
	}
}
/**
 *
 */
void Robot::startCommunicating()
{
	communicating = true;
	CommunicationService::getCommunicationService().runRobotServer(this,12345);
}
/**
 *
 */
void Robot::stopCommunicating()
{
	MessageASIO::Client c1ient( CommunicationService::getCommunicationService().getIOService(), ConfigFile::getInstance().getIpaddress(), ConfigFile::getInstance().getPort(), shared_from_this());
	MessageASIO::Message message( 1, "stop");
	c1ient.dispatchMessage( message);
	Logger::log("stop");
	communicating = false;
}
/**
 *
 */
void Robot::handleRequest( MessageASIO::Message& aMessage)
{
	Logger::log( __PRETTY_FUNCTION__ + std::string( " not implemented, ") + aMessage.asString());
	if(std::string("end").compare(aMessage.asString()) == 0)
	{
		gettingData = false;
		getData(rawRobotData);
		rawRobotData.clear();
	}
	else
	{
		gettingData = true;
	}

	if(gettingData){
		rawRobotData.push_back(aMessage.asString());
	}
	aMessage.setBody( aMessage.asString() + " Server reponse");
}


void Robot::getData(std::vector<std::string>& rawData){
	Logger::log("converting data");
	std::string type = "";
	std::vector<std::string> newData;
	for(auto regel : rawData){
		if(std::string("").compare(type) == 0)
		{
			type = regel;
			continue;
		}
		if(std::string("end").compare(regel) == 0)
		{
			continue;
		}
		std::smatch match;
		std::regex reg("\\:([a-zA-Z]+|[0-9]+)");
		regex_search(regel, match, reg);
		std::string match1(match[1]);
		newData.push_back(match1);
	}
	if(std::string("robot").compare(type) == 0)
	{
		Logger::log("Data to robot" + std::to_string(std::atoi(newData[3].c_str())));
		RobotWorld::getRobotWorld().newRobot(newData[0],Point(std::atoi(newData[1].c_str()),std::atoi(newData[2].c_str())),true,false,std::atoi(newData[3].c_str()));
	}
	if(std::string("wall").compare(type) == 0)
	{
		Logger::log("Data to wall");
		RobotWorld::getRobotWorld().newWall( Point(std::atoi(newData[0].c_str()),std::atoi(newData[1].c_str())), Point(std::atoi(newData[2].c_str()),std::atoi(newData[3].c_str())),true);
	}
	if(std::string("goal").compare(type) == 0)
	{
		Logger::log("Data to goal");
		RobotWorld::getRobotWorld().newGoal( newData[0], Point(std::atoi(newData[1].c_str()),std::atoi(newData[2].c_str())),true,false);
	}
	type = "";
	newData.clear();
}
/**
 *
 */
void Robot::handleResponse( MessageASIO::Message& aMessage)
{
	Logger::log( __PRETTY_FUNCTION__ + std::string( " not implemented, ") + aMessage.asString());
}
/**
 *
 */
std::string Robot::asString() const
{
	std::ostringstream os;

	os << "Robot " << name << " at (" << position.x << "," << position.y << ")";

	return os.str();
}
/**
 *
 */
std::string Robot::asDebugString() const
{
	std::ostringstream os;

	os << "Robot:\n";
	os << AbstractAgent::asDebugString();
	os << "Robot " << name << " at (" << position.x << "," << position.y << ")\n";

	return os.str();
}
/**
 *
 */
void Robot::drive(GoalPtr goal)
{
	try
	{
		Logger::log( __PRETTY_FUNCTION__);
		for (std::shared_ptr< AbstractSensor > sensor : sensors)
		{
			//enables and starts sensor. Starts sensor thread for each sensor.
			sensor->setOn(this,100);
		}

		if (speed == 0.0)
		{
			speed = 10.0;
		}

		unsigned pathPoint = 0;
		while (position.x > 0 && position.x < 500 && position.y > 0 && position.y < 500 && pathPoint < path.size())
		{
			const Vertex& vertex = path[pathPoint+=speed];
			front = Vector( vertex.asPoint(), position);
			position.x = vertex.x;
			position.y = vertex.y;

			if(arrived(goal))
			{
				notifyObservers();
				speed = 0;
				break;
			}
			if (collision())
			{
				notifyObservers();
				break;
			}

			notifyObservers();

			std::this_thread::sleep_for( std::chrono::milliseconds( 50));

			// this should be either the last call in the loop or
			// part of the while.
			if(stop == true)
			{
				return;
			}
		} // while

		for (std::shared_ptr< AbstractSensor > sensor : sensors)
		{
			sensor->setOff();
		}
	}
	catch (std::exception& e)
	{
		std::cerr << __PRETTY_FUNCTION__ << ": " << e.what() << std::endl;
	}
	catch (...)
	{
		std::cerr << __PRETTY_FUNCTION__ << ": unknown exception" << std::endl;
	}
}
/**
 *
 */
void Robot::calculateRoute(GoalPtr aGoal)
{
	path.clear();
	if (aGoal)
	{
		// Turn off logging if not debugging AStar
		Logger::setDisable();

		front = Vector( aGoal->getPosition(), position);
		handleNotificationsFor( astar);
		path = astar.search( position, aGoal->getPosition(), size);
		stopHandlingNotificationsFor( astar);

		Logger::setDisable( false);
	}
}
/**
 *
 */
bool Robot::arrived(GoalPtr aGoal)
{
	if (aGoal && intersects( aGoal->getRegion()))
	{
		return true;
	}
	return false;
}
/**
 *
 */
bool Robot::operator ==(const Robot& b)
{
	return robotId == b.getRobotId();
}
/**
 *
 */
bool Robot::operator !=(const Robot& b)
{
	return robotId != b.getRobotId();
}
/**
 *
 */
bool Robot::collision()
{
	Point frontLeft = getFrontLeft();
	Point frontRight = getFrontRight();
	Point backLeft = getBackLeft();
	Point backRight = getBackRight();

	const std::vector< WallPtr >& walls = RobotWorld::getRobotWorld().getWalls();
	for (WallPtr wall : walls)
	{
		if (Shape2DUtils::intersect( frontLeft, frontRight, wall->getPoint1(), wall->getPoint2()) ||
			Shape2DUtils::intersect( frontLeft, backLeft, wall->getPoint1(), wall->getPoint2())	||
			Shape2DUtils::intersect( frontRight, backRight, wall->getPoint1(), wall->getPoint2()))
		{
			return true;
		}
	}
	return false;
}

void Robot::handleLaserDistanceSensor(bool triggerd)
{
	if(triggerd){
		speed = 0;
	}else{
		speed = 10;
	}
}
