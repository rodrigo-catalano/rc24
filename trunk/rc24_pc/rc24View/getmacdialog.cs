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
    public partial class getmacdialog : Form
    {
        public getmacdialog()
        {
            InitializeComponent();
        }
        public byte[] mac
        {
            get
            {
                string txt = textBoxMac.Text.ToUpper();
                byte[] ret = new byte[8];
                for(int i=0;i<8;i++)
                {
                    ret[i] = byte.Parse(txt.Substring(i*2,2), System.Globalization.NumberStyles.HexNumber);
                }
                return ret;
            }
        }
    }
}
