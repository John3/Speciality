﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Torque3D;
using Torque3D.Engine;
using Torque3D.Util;

namespace Game.Modules.SpectatorGameplay.scripts.server
{
   class GameBord
   {
      private int _gameSizeX;
      private int _gameSizeY;
      public int GameSizeX => _gameSizeX;
      public int GameSizeY => _gameSizeY;
      private bool[,] gameBord;
      private string shape = "data/spectatorGameplay/art/GameShapes/player.dts";
      public GameBord(int sizeX, int sizeY)
      {
         _gameSizeX = sizeX % 2 == 0 ? ++sizeX : sizeX;
         _gameSizeY = sizeY % 2 == 0 ? ++sizeY : sizeY;
         gameBord = new bool[_gameSizeX, _gameSizeY];
         for (int x = 0; x < _gameSizeX; x++)
         {
            for (int y = 0; y < _gameSizeY; y++)
            {
               gameBord[x, y] = false;
            }
         }
      }

      private int PointToX(Point3F point)
      {
         return (int)point.X + _gameSizeX / 2;
      }
      private int PointToY(Point3F point)
      {
         return (int)point.Y + _gameSizeY / 2;
      }

      private Point3F XYToPoint(int x, int y)
      {
         return new Point3F(x - _gameSizeX /2, y - _gameSizeY / 2, 0);
      }
      public bool IsOccupied(Point3F point)
      {
         int x = PointToX(point);
         int y = PointToY(point);
         if (x >= _gameSizeX || x < 0 || y >= _gameSizeY || y < 0)
            return true;
         else return gameBord[x, y];
      } 

      //I use that the shape is has unit size in x and y
      public void AddShape(TSStatic shape)
      {
         Point3F point = shape.Position;
         Point3F scale = shape.Scale;
         int xPos = PointToX(point);
         int yPos = PointToY(point);
         for (int x = xPos - (int) (scale.X / 2); x <= xPos + (int) (scale.X / 2); x++)
         {
            for (int y = yPos - (int) (scale.Y / 2); y <= yPos + (int) (scale.Y / 2); y++)
            {
               if (x >= _gameSizeX || x < 0 || y >= _gameSizeY || y < 0)
               {
                  continue;
               }
               gameBord[x, y] = true;
            }
         } 
      }

      public Point3F PickPlayerSpawn()
      {
         int x = _gameSizeX /2;
         int y = _gameSizeY /2;
         while (gameBord[x, y])
         {
            y = (y + 1) % (_gameSizeY-1);
            if (y == 0)
            {
               x = (x + 1) % (_gameSizeX-1);
            }
         }

         return XYToPoint(x,y);
      }

      public void CreateBoundingBox()
      {
         float rightWallPos = _gameSizeX / 2;
         float leftWallPos = -_gameSizeX / 2;
         float frontWallpos = _gameSizeY / 2;
         float backWallPos = -_gameSizeY / 2;

         TSStatic rightWall = new TSStatic()
         {
            ShapeName = shape,
            Position = XYToPoint(_gameSizeX-1,(_gameSizeY-1)/2),
            CollisionType = TSMeshType.Bounds,
            Scale = new Point3F(1, _gameSizeY, 0.5f)
         };
         TSStatic leftWall = new TSStatic()
         {
            ShapeName = shape,
            Position = XYToPoint(0,(_gameSizeY-1)/2),
            CollisionType = TSMeshType.Bounds,
            Scale = new Point3F(1, GameSizeY, 0.5f)
         };
         TSStatic frontWall = new TSStatic()
         {
            ShapeName = shape,
            Position = XYToPoint((_gameSizeX-1)/2, 0),
            CollisionType = TSMeshType.Bounds,
            Scale = new Point3F(_gameSizeX, 1, 0.5f)
         };
         TSStatic backWall = new TSStatic()
         {
            ShapeName = shape,
            Position = XYToPoint((_gameSizeX-1)/2,_gameSizeY-1),
            CollisionType = TSMeshType.Bounds,
            Scale = new Point3F(_gameSizeX, 1, 0.5f)
         };
         rightWall.registerObject();
         leftWall.registerObject();
         frontWall.registerObject();
         backWall.registerObject();
         AddShape(rightWall);
         AddShape(leftWall);
         AddShape(frontWall);
         AddShape(backWall);

      }

      public void GenerateRandomObstacles(int count)
      {

         Random rand = new Random();
         for (int i = 0; i < count; i++)
         {
            int xpos = rand.Next(0, _gameSizeX-1);
            int ypos = rand.Next(0, _gameSizeY-1);
            int xscale = rand.Next(1, 10);
            int yscale =  rand.Next(1, 10);
            TSStatic obstacle = new TSStatic()
            {
               ShapeName = shape,
               Position = XYToPoint(xpos,ypos),
               CollisionType = TSMeshType.Bounds,
               Scale = new Point3F(xscale, yscale, 0.5f)
            };
            obstacle.registerObject();
            AddShape(obstacle);
         }
      }


   }
}
