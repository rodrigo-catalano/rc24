/*
Copyright 2009 © Graham Bloice

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
using System.Drawing;
using System.Windows.Forms;
using rc24;

namespace Serial
{
    /// <summary>
    /// Displays a progress dialog for an upload operation.
    /// </summary>
    public partial class UploadProgress : Form
    {
        private routedUploader upLoader;
        public UploadProgress(routedUploader newUpLoader)
        {
            InitializeComponent();
            upLoader = newUpLoader;
            upLoader.UploadEvent += delegate(object sender, routedUploader.UploadEventArgs e)
            {
                if (e.percentDone >= 100)
                {
                    this.Hide();                       
                }
                else if (e.percentDone >= 0)
                {
                    UploadProgressBar.Value = e.percentDone;
                }
            };
        }
        
        void UploadProgressLoad(object sender, EventArgs e)
        {
            // Binding pctDoneBind = new Binding("Value", this, "percentDone");
            // UploadProgressBar.DataBindings.Add(pctDoneBind);
        }
        
        void UploadProgressFormClosing(object sender, FormClosingEventArgs e)
        {
            // TODO - remove anynomous event handler
        	// upLoader.UploadEvent -= ??
        }
    }
}
