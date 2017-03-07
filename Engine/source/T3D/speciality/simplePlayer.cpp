#include "simplePlayer.h"
#include "core/stream/bitStream.h"
#include "math/mathIO.h"
#include "materials/baseMatInstance.h"
#include "gfx/gfxDrawUtil.h"
#include "jmorecfg.h"

IMPLEMENT_CO_NETOBJECT_V1(SimplePlayer);
IMPLEMENT_CO_DATABLOCK_V1(SimplePlayerData);

SimplePlayerData::SimplePlayerData()
{
   mMoveSpeed = 1.0f;
   mTurnSpeed = 1.0f;
   mFriction = 0.1f;

   mFOV = 70.0f;
   mAspectRatio = 1.0f;
   mNearDist = 0.5f;
   mFarDist = 200.0f;

   mShootDelay = 15;

   mHealth = 100;
}

SimplePlayerData::~SimplePlayerData()
{

}

void SimplePlayerData::initPersistFields()
{
   addField("MoveSpeed", TYPEID< F32 >(), Offset(mMoveSpeed, SimplePlayerData),
            "");
   addField("TurnSpeed", TYPEID< F32 >(), Offset(mTurnSpeed, SimplePlayerData),
            "");
   addField("Friction", TYPEID< F32 >(), Offset(mFriction, SimplePlayerData),
            "");

   addField("FOV", TYPEID< F32 >(), Offset(mFOV, SimplePlayerData),
            "");
   addField("AspectRatio", TYPEID< F32 >(), Offset(mAspectRatio, SimplePlayerData),
            "");
   addField("NearDist", TYPEID< F32 >(), Offset(mNearDist, SimplePlayerData),
            "");
   addField("FarDist", TYPEID< F32 >(), Offset(mFarDist, SimplePlayerData),
            "");

   addField("ShootDelay", TYPEID< F32 >(), Offset(mShootDelay, SimplePlayerData),
            "");

   addField("Health", TYPEID< F32 >(), Offset(mHealth, SimplePlayerData),
            "");

   Parent::initPersistFields();
}

void SimplePlayerData::onRemove()
{
   Parent::onRemove();
}

bool SimplePlayerData::onAdd()
{
   return Parent::onAdd();
}

void SimplePlayerData::packData(BitStream* stream)
{
   Parent::packData(stream);
   stream->writeInt(mMoveSpeed*1000.0f, 28);
   stream->writeInt(mFriction*1000.0f, 28);
   stream->write(mTurnSpeed);
   stream->write(mFOV);
   stream->write(mAspectRatio);
   stream->write(mNearDist);
   stream->write(mFarDist);
   stream->write(mShootDelay);
   stream->write(mHealth);
}

void SimplePlayerData::unpackData(BitStream* stream)
{
   Parent::unpackData(stream);
   mMoveSpeed = stream->readInt(28)/1000.0f;
   mFriction = stream->readInt(28)/1000.0f;
   stream->read(&mTurnSpeed);
   stream->read(&mFOV);
   stream->read(&mAspectRatio);
   stream->read(&mNearDist);
   stream->read(&mFarDist);
   stream->read(&mShootDelay);
   stream->read(&mHealth);
}


SimplePlayer::SimplePlayer()
{
   mTypeMask |= PlayerObjectType;

   mVelocity = VectorF(0, 0, 0);
   mMovingLeft = 0;
   mMovingRight = 0;
   mMovingForward = 0;
   mMovingBackward = 0;

   mRot = 0.0f;
   mHealth = 20.0f;

   mPrepared = false;
   mLastRot = 0.0f;
   mLastPosX = 0.0f;
   mLastPosY = 0.0f;
   mLastDamageProb = 0.0f;

   mLastHealth = 20.0f;

   mTimeSawEnemy = S32_MAX;
   mTimeTookDamage = S32_MAX;
   mShootDelay = 0;

   mRenderFrustum = false;
   mRenderDistance = false;

   mTickCount = 0;
   mThinkFunction = NULL;

   CollisionMoveMask = (TerrainObjectType | PlayerObjectType |
      StaticShapeObjectType | VehicleObjectType |
      VehicleBlockerObjectType | DynamicShapeObjectType | StaticObjectType | EntityObjectType | TriggerObjectType);
}

SimplePlayer::~SimplePlayer()
{

}

void SimplePlayer::initPersistFields()
{
   addField("MovingForward", TYPEID< S32 >(), Offset(mMovingForward, SimplePlayer),
            "");
   addField("MovingBackward", TYPEID< S32 >(), Offset(mMovingBackward, SimplePlayer),
            "");
   addField("MovingRight", TYPEID< S32 >(), Offset(mMovingRight, SimplePlayer),
            "");
   addField("MovingLeft", TYPEID< S32 >(), Offset(mMovingLeft, SimplePlayer),
            "");
   addField("RenderFrustum", TYPEID< bool >(), Offset(mRenderFrustum, SimplePlayer),
            "");
   addField("RenderDistance", TYPEID< bool >(), Offset(mRenderDistance, SimplePlayer),
            "");
   addField("ThinkFunction", TypeCaseString, Offset(mThinkFunction, SimplePlayer), "");
   addField("Health", TypeF32, Offset(mHealth, SimplePlayer), "");
   addField("Rot", TypeF32, Offset(mRot, SimplePlayer), "");

   Parent::initPersistFields();
}

bool SimplePlayer::onAdd()
{
   if (!Parent::onAdd()) return false;

   // Setup our bounding box
   if (mDataBlock->mShape)
   {
      mObjBox = mDataBlock->mShape->bounds;
   }
   else
   {
      mObjBox = Box3F(Point3F(-1, -1, -1), Point3F(1, 1, 1));
   }

   if (mDataBlock->mShape)
   {
      //mShape = new TSShapeInstance(mDataBlock->shape, true);
   }
   resetWorldBox();

   addToScene();

   if (isServerObject())
   {
      scriptOnAdd();
   }

   if (isServerObject())
      mCollision.prepCollision(this);

   return true;
}

void SimplePlayer::onRemove()
{
   Parent::onRemove();
   removeFromScene();
   mCollision.safeDelete();
   disableCollision();
}

void SimplePlayer::advanceTime(F32 dt)
{
}

F32 SimplePlayer::getDistanceToObstacleInFront(F32& lDist, F32& rDist)
{
   F32 width = mObjBox.maxExtents.x;
   F32 depth = mObjBox.maxExtents.y;
   F32 height = mObjBox.maxExtents.z;
   MatrixF mat = getTransform();
   Point3F leftPoint = Point3F(-width, depth, height / 2);
   Point3F rightPoint = Point3F(width, depth, height / 2);
   mat.mulP(leftPoint);
   mat.mulP(rightPoint);

   Point3F dir = getTransform().getForwardVector();
   RayInfo rInfo;

   lDist = 0.0f; 
   rDist = 0.0f;

   disableCollision();
   if (getContainer()->castRay(leftPoint, leftPoint + (dir * 100), CollisionMoveMask, &rInfo))
      lDist = rInfo.distance;
   if (getContainer()->castRay(rightPoint, rightPoint + (dir * 100), CollisionMoveMask, &rInfo))
      rDist = rInfo.distance;
   enableCollision();

   return lDist > rDist ? lDist : rDist;
}

bool SimplePlayer::canSee(SceneObject* other)
{
   Frustum frustum;
   frustum.set(false, getDataBlock()->getFOV(), getDataBlock()->getAspectRatio(), getDataBlock()->getNearDist(), getDataBlock()->getFarDist(), getTransform());
   if (frustum.isCulled(other->getWorldBox()))
      return false;

   Point3F center = mWorldBox.getCenter();
   
   F32 width = other->getObjBox().maxExtents.x;
   F32 depth = other->getObjBox().maxExtents.y;
   F32 height = other->getObjBox().maxExtents.z;
   MatrixF mat = other->getTransform();
   Point3F leftFrontPoint = Point3F(-width, depth, height / 2);
   Point3F rightFrontPoint = Point3F(width, depth, height / 2);
   Point3F leftBackPoint = Point3F(-width, -depth, height / 2);
   Point3F rightBackPoint = Point3F(width, -depth, height / 2);
   mat.mulP(leftFrontPoint);
   mat.mulP(rightFrontPoint);
   mat.mulP(leftBackPoint);
   mat.mulP(rightBackPoint);

   RayInfo rInfo;

   bool blocked = false;

   U32 mask = CollisionMoveMask & ~PlayerObjectType;

   disableCollision();
   if (  getContainer()->castRay(center, leftFrontPoint, mask, &rInfo)
      && getContainer()->castRay(center, leftBackPoint, mask, &rInfo)
      && getContainer()->castRay(center, rightFrontPoint, mask, &rInfo)
      && getContainer()->castRay(center, rightBackPoint, mask, &rInfo))
         blocked = true;
   enableCollision();

   return !blocked;
}

bool SimplePlayer::trySetRotation(F32 rot)
{
   MatrixF oldTrans = getTransform();
   MatrixF trans = getTransform();
   MatrixF mat;
   mat.set(EulerF(0.0f, 0.0f, rot));
   mat.setPosition(getPosition());
   //AngAxisF::RotateZ(rot, &mat);
   //trans.mul(mat);
   setTransform(mat);
   Point3F vel = Point3F::Zero;
   bool collidingAfter = mCollision.checkCollisions(TickMs, &vel, getPosition());
   if (collidingAfter)
   {
      setTransform(oldTrans);
      return false;
   }
   return true;
}

void SimplePlayer::processTick(const Move* move)
{
   // apply gravity
   Point3F force = Point3F(0, 0, -9.81);

   Point3F moveAcceleration = Point3F(mMovingRight - mMovingLeft, mMovingForward - mMovingBackward, 0) * mDataBlock->getMoveSpeed();

   //MatrixF trans = getTransform();
   MatrixF mat;
   AngAxisF::RotateZ(mRot, &mat);
   mat.mulV(moveAcceleration);
   //trans.mul(mat);
   //setTransform(trans);

   VectorF acceleration = moveAcceleration + force;

   mVelocity += acceleration * TickSec - mVelocity * mDataBlock->getFriction();

   if(standingOnGround())
   {
      mVelocity.z = 0.0f;
   }


   mCollision.update();
   updatePosition(TickSec);

   doThink();
}


ImplementEnumType(SimplePlayerActions, "")
   { SimplePlayer::MoveLeft, "MoveLeft", "\n" },
   { SimplePlayer::MoveRight,     "MoveRight", "\n" },
   { SimplePlayer::MoveForward,     "MoveForward", "\n" },
   { SimplePlayer::MoveBackward,     "MoveBackward", "\n" },
   { SimplePlayer::TurnRight,     "TurnRight", "\n" },
   { SimplePlayer::TurnLeft,     "TurnLeft", "\n" },
   { SimplePlayer::Shoot,     "Shoot", "\n" },
   { SimplePlayer::Prepare,     "Prepare", "\n" }
EndImplementEnumType;

bool SimplePlayer::doThink()
{
   if (isDeleted()) return false;
   if (isServerObject() && mThinkFunction)
   {
      if (!mHasThoughtOnce)
      {
         mHasThoughtOnce = true;
         mLastRot = mRot;
         mLastPosX = getPosition().x;
         mLastPosY = getPosition().y;
         mLastHealth = mHealth;
      }

      if (mPrepared)
      {
         mPrepared = false;
         if(doThink())
         {
            return true;
         }
      } else
      {
         mShootDelay = mShootDelay <= 0 ? 0 : --mShootDelay;
         mTickCount++;
      }

      F32 damageProb = 0.0f; // todo Support more players?
      F32 enemyHealth = 0.0f;

      Con::executef("SetDamageProbText", getName(), damageProb);

      SimGroup *playersGroup = static_cast<SimGroup*>(Sim::findObject("Players"));
      for(int i = 0; i < playersGroup->size(); i++)
      {
         SimplePlayer *player = static_cast<SimplePlayer*>((*playersGroup)[i]);
         if (player->getId() == getId()) continue;
         
         enemyHealth = player->mHealth;
         
         if (!canSee(player)) continue;
         mTimeSawEnemy = mTickCount;
         damageProb = Con::executef("GetDamagePropability", this, player).getFloatValue();
      }

      FeatureVector features;

      features.mDeltaRot = mRot - mLastRot;
      features.mDeltaMovedX = getPosition().x - mLastPosX;
      features.mDeltaMovedY = getPosition().y - mLastPosY;
      features.mVelX = getVelocity().x;
      features.mVelY = getVelocity().y;

      mLastRot = mRot;
      mLastPosX = getPosition().x;
      mLastPosY = getPosition().y;

      features.mDamageProb = damageProb;
      features.mDeltaDamageProb = damageProb - mLastDamageProb;
      getDistanceToObstacleInFront(features.mDistanceToObstacleLeft, features.mDistanceToObstacleRight);
      features.mHealth = mHealth;
      features.mEnemyHealth = enemyHealth;

      mLastDamageProb = damageProb;
      if (mHealth != mLastHealth)
      {
         mTimeTookDamage = mTickCount;
      }
      mLastHealth = mHealth;

      features.mTicksSinceDamage = mTimeTookDamage == S32_MAX ? S32_MAX : mTickCount - mTimeTookDamage;
      features.mTicksSinceObservedEnemy = mTimeSawEnemy == S32_MAX ? S32_MAX : mTickCount - mTimeSawEnemy;
      features.mShootDelay = mShootDelay;
      features.mTickCount = mTickCount;

      Actions action = EngineUnmarshallData< SimplePlayerActions >()(Con::executef(mThinkFunction, features).getStringValue());
      Con::executef("LogPlayerAction", this, features, action);

      switch (action)
      {
      case MoveForward:
         mMovingForward++;
         break;
      case MoveBackward:
         mMovingBackward++;
         break;
      case MoveLeft:
         mMovingLeft++;
         break;
      case MoveRight:
         mMovingRight++;
         break;
      case TurnRight:
         if (trySetRotation(mRot + mDataBlock->getTurnSpeed())) mRot += mDataBlock->getTurnSpeed();
         break;
      case TurnLeft:
         if (trySetRotation(mRot - mDataBlock->getTurnSpeed())) mRot -= mDataBlock->getTurnSpeed();
         break;
      case Shoot:
         if (mShootDelay == 0) {
            mShootDelay = mDataBlock->getShootDelay();
            if(Shoot_callback())
            {
               return true;
            }
         }
         break;
      case Prepare:
         mPrepared = true;
         break;
      default:
         break;
      }

      Point3F moveAcceleration = Point3F(mMovingRight - mMovingLeft, mMovingForward - mMovingBackward, 0) * mDataBlock->getMoveSpeed();

      MatrixF mat;
      AngAxisF::RotateZ(mRot, &mat);
      mat.mulV(moveAcceleration);
      
      mVelocity += moveAcceleration * TickSec - mVelocity * mDataBlock->getFriction();

      if (standingOnGround())
      {
         mVelocity.z = 0.0f;
      }

      mCollision.update();
      updatePosition(TickSec);

      mMovingLeft = 0;
      mMovingRight = 0;
      mMovingForward = 0;
      mMovingBackward = 0;
   }
   return false;
}

IMPLEMENT_CALLBACK(SimplePlayer, Shoot, bool, (), (),
   "");

void SimplePlayer::updatePosition(const F32 travelTime)
{
   Point3F newPos;

   newPos = _move(travelTime);

   setPosition(newPos);
   MatrixF mat;
   mat.set(EulerF(0.0f, 0.0f, mRot));
   mat.setColumn(3, newPos);
   setTransform(mat);
   setMaskBits(TransformMask);
}

void SimplePlayer::prepRenderImage(SceneRenderState* state)
{
   ObjectRenderInst *ori = state->getRenderPass()->allocInst<ObjectRenderInst>();
   ori->renderDelegate.bind(this, &SimplePlayer::debugRenderDelegate);
   ori->type = RenderPassManager::RIT_Editor;
   state->getRenderPass()->addInst(ori);

   Parent::prepRenderImage(state);
}

void SimplePlayer::debugRenderDelegate(ObjectRenderInst* ri, SceneRenderState* state, BaseMatInstance* overrideMat)
{
   GFXDrawUtil* du = GFX->getDrawUtil();
   if(mRenderFrustum)
   {
      MatrixF trans;
      getEyeTransform(&trans);

      Frustum frustum;
      frustum.set(false, getDataBlock()->getFOV(), getDataBlock()->getAspectRatio(), getDataBlock()->getNearDist(), getDataBlock()->getFarDist(), trans);

      du->drawFrustum(frustum, ColorI(200, 70, 50, 150));
   }

   if(mRenderDistance)
   {
      F32 width = mObjBox.maxExtents.x;
      F32 depth = mObjBox.maxExtents.y;
      F32 height = mObjBox.maxExtents.z;
      MatrixF mat = getTransform();
      Point3F leftPoint = Point3F(-width, depth, height / 2);
      Point3F rightPoint = Point3F(width, depth, height / 2);
      mat.mulP(leftPoint);
      mat.mulP(rightPoint);

      Point3F dir = getTransform().getForwardVector();
      RayInfo rInfo;

      disableCollision();
      if (getContainer()->castRay(leftPoint, leftPoint + (dir * 100), CollisionMoveMask, &rInfo))
         du->drawLine(leftPoint, rInfo.point, ColorI::GREEN);
      if (getContainer()->castRay(rightPoint, rightPoint + (dir * 100), CollisionMoveMask, &rInfo))
         du->drawLine(rightPoint, rInfo.point, ColorI::GREEN);
      enableCollision();
   }

   if(true)
   {

      F32 killProb = 0.0f; // todo Support more players?

      SimGroup *playersGroup = static_cast<SimGroup*>(Sim::findObject("Players"));
      for (int i = 0; i < playersGroup->size(); i++)
      {
         SimplePlayer *player = static_cast<SimplePlayer*>((*playersGroup)[i]);
         if (player->getPosition() == getPosition()) continue;
         if (!canSee(player)) continue;
         killProb = Con::executef("GetDamagePropability", this, player).getFloatValue();

         ColorI color = ColorI(255 * (1 - killProb), 255 * killProb, 0);
         du->drawLine(getWorldBox().getCenter(), player->getWorldBox().getCenter(), color);
      }
   }
}

bool SimplePlayer::standingOnGround()
{
   RayInfo rInfo;

   Point3F target = getPosition();
   target.z -= mObjBox.maxExtents.z + 0.005;

   return getContainer()->castRay(getPosition(), target, CollisionMoveMask, &rInfo);
}

bool SimplePlayer::onNewDataBlock(GameBaseData* dptr, bool reload)
{
   mDataBlock = dynamic_cast< SimplePlayerData* >(dptr);
   if (!mDataBlock || !Parent::onNewDataBlock(dptr, reload))
      return false;

   mHealth = mDataBlock->getHealth();

   scriptOnNewDataBlock();
   return true;
}

U32 SimplePlayer::packUpdate(NetConnection* conn, U32 mask, BitStream* stream)
{
   U32 retMask = Parent::packUpdate(conn, mask, stream);

   if(stream->writeFlag(mask & InitialUpdateMask))
   {
      stream->writeString(mThinkFunction);
      stream->writeFlag(mRenderFrustum);
      stream->writeFlag(mRenderDistance);
   }

   if(stream->writeFlag(mask & TransformMask))
   {
      mathWrite(*stream, getTransform());
      mathWrite(*stream, getScale());
      stream->write(mRot);
   }

   return retMask;
}

void SimplePlayer::unpackUpdate(NetConnection* conn, BitStream* stream)
{
   Parent::unpackUpdate(conn, stream);

   if(stream->readFlag())
   {
      char buffer[256];
      stream->readString(buffer);
      mThinkFunction = buffer;
      mRenderFrustum = stream->readFlag();
      mRenderDistance = stream->readFlag();
   }

   if(stream->readFlag())
   {
      MatrixF temp;
      Point3F tempScale;
      mathRead(*stream, &temp);
      mathRead(*stream, &tempScale);
      stream->read(&mRot);

      setScale(tempScale);
      setTransform(temp);
   }
}

DefineEngineMethod(SimplePlayer, CanSee, bool, (SceneObject* other), , "")
{
   return object->canSee(other);
}

DefineEngineMethod(SimplePlayer, GetEulerRotation, Point3F, (), , "")
{
   return object->getTransform().toEuler();
}

FeatureVector::FeatureVector()
{
}

FeatureVector::~FeatureVector()
{
}


IMPLEMENT_STRUCT(FeatureVector,
   FeatureVector, MathTypes,
   "")
END_IMPLEMENT_STRUCT;



//-----------------------------------------------------------------------------
// TypePoint3F
//-----------------------------------------------------------------------------
ConsoleType(FeatureVector, TypeFeatureVector, FeatureVector, "")
ImplementConsoleTypeCasters(TypeFeatureVector, FeatureVector)

ConsoleGetType(TypeFeatureVector)
{
   FeatureVector *v = (FeatureVector *)dptr;
   static const U32 bufSize = 256;
   char* returnBuffer = Con::getReturnBuffer(bufSize);
   dSprintf(returnBuffer, bufSize, 
      "%g %g %g %g %g %g %g %g %g %g %g %d %d %d %d", 
      v->mDeltaRot, v->mDeltaMovedX, v->mDeltaMovedY,
      v->mVelX, v->mVelY, v->mDamageProb, v->mDeltaDamageProb,
      v->mDistanceToObstacleLeft, v->mDistanceToObstacleRight, v->mHealth, v->mEnemyHealth,
      v->mTickCount, v->mTicksSinceObservedEnemy, v->mTicksSinceDamage,
      v->mShootDelay);
   return returnBuffer;
}

ConsoleSetType(TypeFeatureVector)
{
   FeatureVector *v = ((FeatureVector *)dptr);
   if (argc == 1)
      dSscanf(argv[0],
         "%g %g %g %g %g %g %g %g %g %g %g %d %d %d %d",
         &v->mDeltaRot, &v->mDeltaMovedX, &v->mDeltaMovedY,
         &v->mVelX, &v->mVelY, &v->mDamageProb, &v->mDeltaDamageProb,
         &v->mDistanceToObstacleLeft, v->mDistanceToObstacleRight, &v->mHealth, &v->mEnemyHealth,
         &v->mTickCount, &v->mTicksSinceObservedEnemy, &v->mTicksSinceDamage,
         &v->mShootDelay);
   else if (argc == 15)
   {
      FeatureVector vector;
      vector.mDeltaRot = dAtof(argv[0]);
      vector.mDeltaMovedX = dAtof(argv[1]);
      vector.mDeltaMovedY = dAtof(argv[2]);
      vector.mVelX = dAtof(argv[3]);
      vector.mVelY = dAtof(argv[4]);
      vector.mDamageProb = dAtof(argv[5]);
      vector.mDeltaDamageProb = dAtof(argv[6]);
      vector.mDistanceToObstacleLeft = dAtof(argv[7]);
      vector.mDistanceToObstacleRight = dAtof(argv[8]);
      vector.mHealth = dAtof(argv[9]);
      vector.mEnemyHealth = dAtof(argv[10]);
      vector.mTickCount = dAtoi(argv[11]);
      vector.mTicksSinceObservedEnemy = dAtoi(argv[12]);
      vector.mTicksSinceDamage = dAtoi(argv[13]);
      vector.mShootDelay = dAtoi(argv[14]);
      *v = vector;
   }
   else
      Con::printf("FeatureVector must be set as { ..14.. } or \"..14..\"");
}

///////////////////////////////////////////////////////////////////////////////////
//  COLLISION  ////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////


void SimplePlayer::handleCollision(Collision& col, VectorF velocity)
{
   if (col.object && (mCollision.getContactInfo().contactObject == NULL ||
      col.object->getId() != mCollision.getContactInfo().contactObject->getId()))
   {
      queueCollision(col.object, velocity - col.object->getVelocity());

      //do the callbacks to script for this collision
      if (isMethod("onCollision"))
      {
         S32 matId = col.material != NULL ? col.material->getMaterial()->getId() : 0;
         Con::executef(this, "onCollision", col.object, col.normal, col.point, matId, velocity);
      }

      if (isMethod("onCollisionEvent"))
      {
         S32 matId = col.material != NULL ? col.material->getMaterial()->getId() : 0;
         Con::executef(this, "onCollisionEvent", col.object, col.normal, col.point, matId, velocity);
      }
   }
}

Point3F SimplePlayer::_move(const F32 travelTime)
{

   // Try and move to new pos
   F32 totalMotion = 0.0f;

   Point3F start;
   Point3F initialPosition;
   getTransform().getColumn(3, &start);
   initialPosition = start;

   VectorF firstNormal(0.0f, 0.0f, 0.0f);
   F32 time = travelTime;
   U32 count = 0;
   S32 sMoveRetryCount = 5;

   mCollision.getCollisionList().clear();

   for (; count < sMoveRetryCount; count++)
   {
      F32 speed = mVelocity.len();
      if (!speed)
         break;

      Point3F end = start + mVelocity * time;

      bool collided = mCollision.checkCollisions(time, &mVelocity, start);

      if (mCollision.getCollisionList().getCount() != 0 && mCollision.getCollisionList().getTime() < 1.0f)
      {
         // Set to collision point
         F32 velLen = mVelocity.len();

         F32 dt = time * getMin(mCollision.getCollisionList().getTime(), 1.0f);
         start += mVelocity * dt;
         time -= dt;

         totalMotion += velLen * dt;

         // Back off...
         if (velLen > 0.f)
         {
            F32 newT = getMin(0.01f / velLen, dt);
            start -= mVelocity * newT;
            totalMotion -= velLen * newT;
         }

         const Collision *collision = &mCollision.getCollisionList()[0];
         const Collision *cp = collision + 1;
         const Collision *ep = collision + mCollision.getCollisionList().getCount();
         for (; cp != ep; cp++)
         {
            if (cp->faceDot > collision->faceDot)
               collision = cp;
         }

         F32 bd = -mDot(mVelocity, collision->normal);

         // Subtract out velocity
         F32 sNormalElasticity = 0.01f;
         VectorF dv = collision->normal * (bd + sNormalElasticity);
         mVelocity += dv;
         if (count == 0)
         {
            firstNormal = collision->normal;
         }
         else
         {
            if (count == 1)
            {
               // Re-orient velocity along the crease.
               if (mDot(dv, firstNormal) < 0.0f &&
                  mDot(collision->normal, firstNormal) < 0.0f)
               {
                  VectorF nv;
                  mCross(collision->normal, firstNormal, &nv);
                  F32 nvl = nv.len();
                  if (nvl)
                  {
                     if (mDot(nv, mVelocity) < 0.0f)
                        nvl = -nvl;
                     nv *= mVelocity.len() / nvl;
                     mVelocity = nv;
                  }
               }
            }
         }
      }
      else
      {
         start = end;
         break;
      }
   }

   if (count == sMoveRetryCount)
   {
      // Failed to move
      start = initialPosition;
      mVelocity.set(0.0f, 0.0f, 0.0f);
   }

   return start;
}
