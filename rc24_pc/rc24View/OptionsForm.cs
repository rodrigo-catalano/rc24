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
using System.IO;
using System.Windows.Forms;

namespace Serial
{
    /// <summary>
    /// Form to set the program optons.
    /// </summary>
    public partial class OptionsForm : Form
    {
        // The user settings
        private MyUserSettings mus;

        public OptionsForm()
        {
            //
            // The InitializeComponent() call is required for Windows Forms designer support.
            //
            InitializeComponent();
        }
        
        void OptTxPathButtonClick(object sender, EventArgs e)
        {
            string newBinFile = getBinFile("Select Tx binary file", txBinPathTextBox.Text);
            if (null != newBinFile)
            {
                txBinPathTextBox.Text = newBinFile;
            }
        }
        
        void OptRxPathButtonClick(object sender, EventArgs e)
        {
            string newBinFile = getBinFile("Select Rx binary file", rxBinPathTextBox.Text);
            if (null != newBinFile)
            {
                rxBinPathTextBox.Text = newBinFile;
            }
        }
        
        void OptTx5148PathButtonClick(object sender, EventArgs e)
        {
            string newBinFile = getBinFile("Select Tx 5148 binary file", tx5148BinPathTextBox.Text);
            if (null != newBinFile)
            {
                tx5148BinPathTextBox.Text = newBinFile;
            }
        }
        
        void OptRx5148PathButtonClick(object sender, EventArgs e)
        {
            string newBinFile = getBinFile("Select Rx 5148 binary file", rx5148BinPathTextBox.Text);
            if (null != newBinFile)
            {
                rx5148BinPathTextBox.Text = newBinFile;
            }
        }

        void CancelButtonClick(object sender, EventArgs e)
        {
            Close();
        }
        
        void OptOKButtonClick(object sender, EventArgs e)
        {
        	// Write the comport back to the settings
        	mus.comPort = String.Concat("COM", comPortUpDown.Text);
        	mus.Save();
        	Close();
        }

        /// <summary>
        /// Function to display a file open dialog to select a binary file 
        /// </summary>
        private string getBinFile(string title, string startPath)
        {
            string retVal = null;
            OpenFileDialog ofd = new OpenFileDialog();
            ofd.InitialDirectory = Path.GetDirectoryName(startPath);
            ofd.Title = title;
            ofd.Filter = "bin files (*.bin)|*.bin|All files (*.*)|*.*" ;
            ofd.RestoreDirectory = true ;

            if(ofd.ShowDialog() == DialogResult.OK)
            {
                retVal = ofd.FileName;
            }
            
            return retVal;
        }
        
        void OptionsFormLoad(object sender, EventArgs e)
        {
            // Instantiate the user settings
            mus = new MyUserSettings();

            // Bind the controls to the settings
            Binding bindTxPath = new Binding("Text", mus, "txbinpath", true, DataSourceUpdateMode.OnPropertyChanged);
            txBinPathTextBox.DataBindings.Add(bindTxPath);
            Binding bindRxPath = new Binding("Text", mus, "rxbinpath", true, DataSourceUpdateMode.OnPropertyChanged);
            rxBinPathTextBox.DataBindings.Add(bindRxPath);
            Binding bindTx5148Path = new Binding("Text", mus, "tx5148binpath", true, DataSourceUpdateMode.OnPropertyChanged);
            tx5148BinPathTextBox.DataBindings.Add(bindTx5148Path);
            Binding bindRx5148Path = new Binding("Text", mus, "rx5148binpath", true, DataSourceUpdateMode.OnPropertyChanged);
            rx5148BinPathTextBox.DataBindings.Add(bindRx5148Path);
            
            // Get the port number
            string comPort = mus.comPort;
            if (null != comPort && comPort.Length > 3)
            {
                comPortUpDown.Value = Convert.ToInt16(comPort.Substring(3, 1));
            }
        }
    }
}
