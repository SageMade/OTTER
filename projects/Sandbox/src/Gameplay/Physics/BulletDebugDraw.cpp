#pragma once
#include "Gameplay/Physics/BulletDebugDraw.h"
#include "Utils/GlmBulletConversions.h"
#include "Logging.h"
#include "Graphics/DebugDraw.h"

BulletDebugDraw::BulletDebugDraw() {
}

void BulletDebugDraw::drawLine(const btVector3& from, const btVector3& to, const btVector3& color) {
	DebugDrawer::Get().DrawLine(ToGlm(from), ToGlm(to), ToGlm(color));
}

void BulletDebugDraw::drawLine(const btVector3& from, const btVector3& to, const btVector3& fromColor, const btVector3& toColor)
{
	DebugDrawer::Get().DrawLine(ToGlm(from), ToGlm(to), ToGlm(fromColor), ToGlm(toColor));
}

void BulletDebugDraw::drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance,
									   int lifeTime, const btVector3& color) {

}

void BulletDebugDraw::reportErrorWarning(const char* warningString) {
	LOG_WARN(warningString);
}

void BulletDebugDraw::draw3dText(const btVector3& location, const char* textString) {

}

void BulletDebugDraw::setDebugMode(int debugMode)
{
	m_debugMode = debugMode;
}
