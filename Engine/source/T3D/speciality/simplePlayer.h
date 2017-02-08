//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#ifndef _SIMPLEPLAYER_H_
#define _SIMPLEPLAYER_H_

#ifndef _SHAPEBASE_H_
#include "T3D/shapeBase.h"
#endif
#include "T3D/physics/physicsBody.h"
#include "simplePlayerCollision.h"

class SimplePlayerData : public ShapeBaseData {
   typedef ShapeBaseData Parent;

public:
   DECLARE_CONOBJECT(SimplePlayerData);

   SimplePlayerData();
   ~SimplePlayerData();

   static void initPersistFields();

   void onRemove() override;
   bool onAdd() override;
   void packData(BitStream* stream) override;
   void unpackData(BitStream* stream) override;

   F32 getMoveSpeed() const { return mMoveSpeed; }
   F32 getFriction() const { return mFriction; }
   F32 getFOV() const { return mFOV; }
   F32 getAspectRatio() const { return mAspectRatio; }
   F32 getNearDist() const { return mNearDist; }
   F32 getFarDist() const { return mFarDist; }

private:
   F32 mMoveSpeed;
   F32 mFriction;

   F32 mFOV;
   F32 mAspectRatio;
   F32 mNearDist;
   F32 mFarDist;
};

class SimplePlayer : public ShapeBase {

	typedef ShapeBase Parent;

public:
   DECLARE_CONOBJECT(SimplePlayer);

   SimplePlayer();
   ~SimplePlayer();

   static void initPersistFields();

   bool onAdd() override;
   void onRemove() override;

   void advanceTime(F32 dt) override;
   void doThink();
   void updatePosition(const F32 travelTime);
   void processTick(const Move* move) override;

   bool onNewDataBlock(GameBaseData* dptr, bool reload) override;

   U32 packUpdate(NetConnection* conn, U32 mask, BitStream* stream) override;
   void unpackUpdate(NetConnection* conn, BitStream* stream) override;

   Point3F getVelocity() const override { return mVelocity; }

   SimplePlayerData* getDataBlock() { return mDataBlock; }

   bool standingOnGround();

   void prepRenderImage(SceneRenderState* state) override;
   void debugRenderDelegate(ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance* overrideMat);

private:
   VectorF mVelocity;
   bool mMovingLeft;
   bool mMovingRight;
   bool mMovingForward;
   bool mMovingBackward;

   bool mRenderFrustum;

   S32 mTickCount;

   StringTableEntry mThinkFunction;

   SimplePlayerData* mDataBlock;

   enum SimplePlayerMasks
   {
      TransformMask = BIT(0)
   };

public:
   enum Actions
   {
      None = 0,
      MoveLeft,
      MoveRight,
      MoveForward,
      MoveBackward
   };

   // Collision
private:
   SimplePlayerCollision mCollision;
   
   Point3F _move(const F32 travelTime);
public:
   U32 CollisionMoveMask;
   void handleCollision(Collision &col, VectorF velocity);
};

typedef SimplePlayer::Actions SimplePlayerActions;

DefineEnumType(SimplePlayerActions);

class FeatureVector : public SimObject {

	typedef SimObject Parent;

public:
   DECLARE_CONOBJECT(FeatureVector);

   FeatureVector();
   ~FeatureVector();

   static void initPersistFields();

   S32 mTickCount;
};

#endif // _SIMPLEPLAYER_H_
