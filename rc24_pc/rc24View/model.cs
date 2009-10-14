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
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace Serial
{
    public partial class model : Form
    {
        public model()
        {
            InitializeComponent();
        }
        int[] op=new int[4096];
        int[] lut1 = new int[17];
        int[] lutexpo = new int[17];

        private int interp(int x, int[] table)
        {
            x -= -2048;
            int lIdx = x >> 8;

            int ly = table[lIdx];
            int uy = table[lIdx + 1];
            return ly + ((uy - ly) * (x - (lIdx << 8)) >> 8);

        }
        private void calc()
        {
            int r=(int)rate.Value;
            int exp=-(int)expo.Value;
            if (exp < 0) exp = exp * 57 / 100;


            for (int ii = 0; ii < 17; ii++)
            {
                int i = ii*256-2048;
                //lut1[ii] = (r + (r * exp / 100)) * i / 100 - i * i / 8 * i / 100 * exp / 524288 * r / 100;

                lut1[ii] = ((100-exp) * i / 100 + i * i / 8 * i / 100 * exp / 524288) * r / 100;
                
                
                lutexpo[ii] = - i * i / 8 * i / 100 * exp / 524288;
          

            }
            for(int i=-2048;i<2048;i++)
            {
//                op[i+2048]=(r+(r*exp/100))*i/100 - i*i/8*i/100*exp/524288*r/100;
//                op[i + 2048] = - i * i / 8 * i / 100 * exp / 524288;
                op[i + 2048] = interp(i, lut1);
            }
            graph1.data = op;
            graph1.Invalidate();

        }

        private void rate_ValueChanged(object sender, EventArgs e)
        {
            calc();
        }

        private void expo_ValueChanged(object sender, EventArgs e)
        {
            calc();
        }

        private void model_Load(object sender, EventArgs e)
        {

        }
        
    }
}
