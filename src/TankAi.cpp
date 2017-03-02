#include "TankAi.h"


TankAi::TankAi(std::vector<sf::CircleShape> const & obstacles,std::vector<sf::CircleShape> const & node, entityx::Entity::Id id) 
	: m_aiBehaviour(AiBehaviour::SEEK_PLAYER), m_steering(0,0), m_obstacles(obstacles), m_node(node), m_currentNode(0)
{

}

void TankAi::update(entityx::Entity::Id playerId,
	entityx::Entity::Id aiId,
	entityx::Entity::Id nodeId,
	int &currentNode,
	entityx::EntityManager& entities,
	double dt)
{
	entityx::Entity aiTank = entities.get(aiId);
	Motion::Handle motion = aiTank.component<Motion>();
	Position::Handle position = aiTank.component<Position>();

	sf::Vector2f vectorToPlayer = seek(playerId, aiId, entities);

	sf::Vector2f vectorToNode = path(aiId, entities);

	switch (m_aiBehaviour)
	{
	case AiBehaviour::SEEK_PLAYER:
		m_steering += thor::unitVector(vectorToPlayer);
		m_steering += collisionAvoidance(aiId, entities);
		m_steering = Math::truncate(m_steering, MAX_FORCE);
		m_velocity = Math::truncate(m_velocity + m_steering, MAX_SPEED);

		break;
	case AiBehaviour::STOP:
		motion->m_speed = 0;

	case AiBehaviour::PATH_FOLLOWING:
		m_steering += thor::unitVector(vectorToNode);
		m_steering += collisionAvoidance(aiId, entities);
		m_steering = Math::truncate(m_steering, MAX_FORCE);
		m_velocity = Math::truncate(m_velocity + m_steering, MAX_SPEED);
	default:
		break;
	}

	// Now we need to convert our velocity vector into a rotation angle between 0 and 359 degrees.
	// The m_velocity vector works like this: vector(1,0) is 0 degrees, while vector(0, 1) is 90 degrees.
	// So for example, 223 degrees would be a clockwise offset from 0 degrees (i.e. along x axis).
	// Note: we add 180 degrees below to convert the final angle into a range 0 to 359 instead of -PI to +PI
	auto dest = atan2(-1 * m_velocity.y, -1 * m_velocity.x) / thor::Pi * 180 + 180; 

	auto currentRotation = position->m_rotation;	

	// Find the shortest way to rotate towards the player (clockwise or anti-clockwise)
	if (std::round(currentRotation - dest) == 0.0)
	{
		m_steering.x = 0;
		m_steering.y = 0;
	}
	else if ((static_cast<int>(std::round(dest - currentRotation + 360))) % 360 < 180)
	{
		// rotate clockwise
		position->m_rotation += 1;
	}
	else
	{
		// rotate anti-clockwise
		position->m_rotation -= 1;
	}

	
	if (thor::length(vectorToNode) < MAX_SEE_AHEAD)
	{
		m_aiBehaviour = AiBehaviour::STOP;
		currentNode++;
		if (currentNode >= 20/*m_node.size()*/)
		{
			currentNode = 0;
		}
	}	
	else
	{
		motion->m_speed = thor::length(m_velocity);	
		m_aiBehaviour = AiBehaviour::PATH_FOLLOWING;
	}

}

sf::Vector2f TankAi::seek(entityx::Entity::Id playerId, entityx::Entity::Id aiId, entityx::EntityManager& entities) const
{
	sf::Vector2f searchingVec;

	entityx::Entity aiTank = entities.get(aiId);
	Position::Handle aiPos = aiTank.component<Position>();

	entityx::Entity playerTank = entities.get(playerId);
	Position::Handle playerPos = playerTank.component<Position>();

	searchingVec = playerPos->m_position - aiPos->m_position;

	return searchingVec;
}

sf::Vector2f TankAi::path(entityx::Entity::Id aiId, entityx::EntityManager& entities) const
{
	sf::Vector2f followVec;

	entityx::Entity aiTank = entities.get(aiId);
	Position::Handle aiPos = aiTank.component<Position>();

	followVec = m_node.at(m_currentNode).getPosition()- aiPos->m_position;

	return followVec;
}

sf::Vector2f TankAi::collisionAvoidance(entityx::Entity::Id aiId, entityx::EntityManager& entities)
{
	entityx::Entity aiTank = entities.get(aiId);
	Position::Handle aiPos = aiTank.component<Position>();	

	auto headingRadians = thor::toRadian(aiPos->m_rotation);	
	sf::Vector2f headingVector(std::cos(headingRadians) * MAX_SEE_AHEAD, std::sin(headingRadians) * MAX_SEE_AHEAD);

	m_ahead = aiPos->m_position + headingVector;	
	m_halfAhead = aiPos->m_position + (headingVector * 0.5f);

	const sf::CircleShape mostThreatening = findMostThreateningObstacle(aiId, entities);
	sf::Vector2f avoidance(0, 0);

	if (mostThreatening.getRadius() != 0.0)
	{
		auto threatPos = mostThreatening.getPosition();
		auto mypos = aiPos->m_position;
		avoidance.x = m_ahead.x - mostThreatening.getPosition().x;
		avoidance.y = m_ahead.y - mostThreatening.getPosition().y;
		avoidance = thor::unitVector(avoidance);
		avoidance *= MAX_AVOID_FORCE;
	}
	else
	{
		avoidance *= 0.0f;
	}
    return avoidance;
}

//private function findMostThreateningObstacle() :Obstacle{
//		var mostThreatening : Obstacle = null;
//
//	for (var i : int = 0; i < Game.instance.obstacles.length; i++) 
//	{
//			var obstacle : Obstacle = Game.instance.obstacles[i];
//			var collision : Boolean = lineIntersecsCircle(ahead, ahead2, obstacle);
//
//	// "position" is the character's current position
//		if (collision && (mostThreatening == null || distance(position, obstacle) < distance(position, mostThreatening))) 
//		{
//			mostThreatening = obstacle;
//		}
//	}
//	return mostThreatening;
//}

const sf::CircleShape TankAi::findMostThreateningObstacle(entityx::Entity::Id aiId, entityx::EntityManager& entities) 
{		
	sf::CircleShape mostThreat;
	mostThreat.setRadius(0);
	entityx::Entity aiTank = entities.get(aiId);
	Position::Handle aiPos = aiTank.component<Position>();

	for (int i = 0; i < m_obstacles.size(); i++)
	{
		bool collision = Math::lineIntersectsCircle(m_ahead, m_halfAhead, m_obstacles[i]);

		if (collision && (mostThreat.getRadius()==0.0f|| Math::distance(aiPos->m_position,m_obstacles[i].getPosition()) < Math::distance(aiPos->m_position, mostThreat.getPosition())))
		{
			mostThreat = m_obstacles[i];
		}
	}

	return mostThreat;
}

