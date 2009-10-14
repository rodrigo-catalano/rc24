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

namespace Serial
{
    public partial class graph : UserControl
    {
        public graph()
        {
            InitializeComponent();
        }
        public int[] data;
        protected override void OnPaintBackground(PaintEventArgs pevent)
        {

        }
        protected override void OnPaint(PaintEventArgs e)
        {
            base.OnPaint(e);
           Graphics g = e.Graphics;
           g.FillRectangle(Brushes.Cornsilk, this.ClientRectangle);
                     
            if(data!=null && data.Length>1)
            {
                int lastx=0;
                int lasty =  this.Height / 2 - data[0] / 8;

                for (int x = 0; x < data.Length; x++)
                {
                    int xx = x * this.Width / data.Length;
                    int yy =  this.Height / 2 - data[x] / 8;
                    g.DrawLine(Pens.Blue, lastx, lasty,xx , yy);
                    lastx=xx;
                    lasty = yy;
                }
            }

        }
    }
}
