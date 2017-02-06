﻿using Torque3D;
using Torque3D.Engine;

namespace Game.Core
{
   //---------------------------------------------------------------------------------------------
   // In the DisplayCursorCanvas package we add some additional functionality to the built-in
   // GuiCanvas class, of which the global Canvas object is an instance. In this case, the behavior
   // we want is for the cursor to automatically display, except when the only guis visible want no
   // cursor - usually the in game interface.
   //---------------------------------------------------------------------------------------------
   [ConsoleClass]
   public class DisplayCursorCanvas : GuiCanvas
   {
      public DisplayCursorCanvas(string pName, bool pRegister = false) : base(pName, pRegister)
      {
      }

      //---------------------------------------------------------------------------------------------
      // checkCursor
      // The checkCursor method iterates through all the root controls on the GameCanvas checking each
      // ones noCursor property. If the noCursor property exists as anything other than false or an
      // empty string on every control, the cursor will be hidden.
      //---------------------------------------------------------------------------------------------
      public void checkCursor()
      {
         int count = getCount();
         for (uint i = 0; i < count; i++)
         {
            GuiControl control = new GuiControl(getObject(i));
            string noCursor = control.getFieldValue("noCursor");
            if (string.IsNullOrEmpty(noCursor))
            {
               Cursor.showCursor();
               return;
            }
         }

         // If we get here, every control requested a hidden cursor, so we oblige.
         Cursor.hideCursor();
      }

      //---------------------------------------------------------------------------------------------
      // The following functions override the GuiCanvas defaults that involve changing the content
      // of the Canvas. Basically, all we are doing is adding a call to checkCursor to each one.
      //---------------------------------------------------------------------------------------------
      public void setContent(GuiControl ctrl)
      {
         base.setContent(ctrl);
         //Parent::setContent(%this, %ctrl);
         checkCursor();
      }

      public void pushDialog(string ctrl, int layer=0, bool center=false)
      {
         base.pushDialog(ctrl, layer, center);
         checkCursor();
      }

      public void popDialog(GuiControl ctrl = null)
      {
         base.popDialog(ctrl);
         checkCursor();
      }

      public void popLayer(string layer)
      {
         base.popLayer(int.Parse(layer));
         checkCursor();
      }
   }
}