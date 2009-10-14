/*
Copyright 2008 - 2009 © Alan Hopper

	This file is part of rc24.

    rc24 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    rc24 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with rc24.  If not, see <http://www.gnu.org/licenses/>.


*/
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.Threading;

namespace Serial
{
    public partial class lcd : UserControl
    {
        Bitmap bmp = new Bitmap(128, 64, System.Drawing.Imaging.PixelFormat.Format24bppRgb);
      
        public lcd()
        {
            InitializeComponent();
        }
        public void UpdateBlock(int block,byte[] pixels,int start)
        {
            int yy=block/8*8;
            int xx=block%8*16;
         
            for(int x=0;x<16;x++)
            {
                byte b=pixels[x+start];
                byte mask=0x01;
                for (int y = 0; y < 8; y++)
                {
                    if ((b & mask) > 0) bmp.SetPixel(xx, yy+y, Color.Black);
                    else bmp.SetPixel(xx, yy+y, Color.YellowGreen);
                    mask <<= 1;
                }
                xx++;
            }
            Invalidate(new Rectangle((xx-16)*2-1,yy*2-1,34,18));
        }
        protected override void OnPaintBackground(PaintEventArgs pevent)
        {

        }
        protected override void OnPaint(PaintEventArgs e)
        {
            //base.OnPaint(e);
            
            Graphics g = e.Graphics;
            
            if (bmp != null)g.DrawImage(bmp, 0, 0, 256, 128);
           
        }
    }
}
