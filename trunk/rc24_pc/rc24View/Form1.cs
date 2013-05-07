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
using System.IO.Ports;
using System.IO;
using System.Threading;
using System.Diagnostics;
using rc24;


/* This communicates with jennic modules through a logic level serial port.
 * The communications protocol is based on that used by jennic for their
 * flash programmer. See JN-AN-1007-Boot-Loader-Serial-Protocol-1v7.pdf
 * 
 * This program can be used inplace of the jennic programmer for programming
 * It will program both in bootload mode or with the module tx app code
 * running
 * 
 * The rx can be reprogrammed over the radio link from a tx connected to the 
 * serial port once the rx has the rx code initially loaded via the bootloader.
 * 
 */


namespace Serial
{
    public partial class Form1 : Form , routedConnector
    {
        private MyUserSettings mus;
        private SerialPort inp;
        private SerialDataReceivedEventHandler serEh;
        
        bootLoaderUploader blupload = new bootLoaderUploader();
        rc24.routedUploader rUpload = null;
        UploadProgress ulProg = null;
        private loopBackCommsTest loopTest = null;

        //      private BackgroundWorker backgroundWorker1;

        delegate void SetTextCallback(string text);
        delegate void jennicMessageCallback(byte[] buff);


        byte[] jennicMsgbuff = new byte[256];
        byte jennicMsgLen = 0;
        byte jennicMsgNextIdx = 0;

        const byte SERIALCON = 0;

        DateTime lastData = DateTime.Now;

        routedTreeWalker treeWalker;

        routedNode pcNode = new routedNode("PC");
        routedNode activeNode;

        dynamic PC;
        bool logComms = false;

        CC_paramBinder nodeParameterList = new CC_paramBinder();

        public Form1()
        {
            InitializeComponent();

            lcd1.MouseClick += new MouseEventHandler(lcd1_MouseClick);
            lcd1.Gesture += new MouseEventHandler(lcd1_Gesture);

            nodeParameterList.SetValue += new Flobbster.Windows.Forms.PropertySpecEventHandler(nodeParameterList_SetValue);
            propertyGrid1.SelectedObject = nodeParameterList;

            //redirect console output to textbox
            TextWriter _writer = new TextBoxStreamWriter(textBox1);
            Console.SetOut(_writer);

            PC = new routedObject() { node = pcNode, con = this };
            
            
            scriptEditor1.PC = PC;

        }

       
        

        void nodeParameterList_SetValue(object sender, Flobbster.Windows.Forms.PropertySpecEventArgs e)
        {
            string paramName = e.Property.Name;

            ccParameter param = activeNode.properties[paramName];
            byte[] cmd=param.buildSetCmd();

            if (cmd != null)
            {
                routedMessage msgValSet = new routedMessage(activeNode.address, cmd);
                SendRoutedMessage(msgValSet, SERIALCON);
                Debug.WriteLine(activeNode.name + ": set parameter: " + paramName + " to: " + param.Value.ToString());
            }
        }
        
        void lcd1_MouseClick(object sender, MouseEventArgs e)
        {
            byte[] cmd = new byte[3];
            cmd[0] = 0xa0;
            cmd[1] = (byte)(e.X / 2);
            cmd[2] = (byte)(e.Y / 2);
            routedMessage msg = new routedMessage(route.DirectLink, cmd);
            SendRoutedMessage(msg, SERIALCON);
        }
        void lcd1_Gesture(object sender, MouseEventArgs e)
        {
            byte[] cmd = new byte[3];
            cmd[0] = 0xa2;
            sbyte dx = (sbyte)(e.X / 2);
            sbyte dy = (sbyte)(e.Y / 2);
            cmd[1] = (byte)dx;
            cmd[2] = (byte)dy;
            routedMessage msg = new routedMessage(route.DirectLink, cmd);
            SendRoutedMessage(msg, SERIALCON);
        }

        void inp_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {

            int i = inp.BytesToRead;
            // packet timeout 
            DateTime t = DateTime.Now;
            if (t.Subtract(lastData).TotalMilliseconds > 200)
            {
                if (jennicMsgNextIdx > 0)
                {
                    SetText("#");
                }
                jennicMsgNextIdx = 0;
            }
            lastData = t;

            for (int n = 0; n < i; n++)
            {
                int c = inp.ReadByte();

                if (jennicMsgNextIdx == 0)
                {
                    jennicMsgLen = (byte)c;
                }
                jennicMsgbuff[jennicMsgNextIdx++] = (byte)c;


                if (jennicMsgNextIdx == jennicMsgLen + 1)
                {
                    if (jennicMsgLen > 0)
                    {
                        //if response clear buff and lock line

                        /*     if ((((jennicMsgbuff[1] & 0x01) == 0 || jennicMsgbuff[1] == 0x2d)) && jennicMsgbuff[1] != 0x2c && (jennicMsgbuff[2] == 0))
                             {
                                 inp.RtsEnable = true;
                                 inp.ReadExisting();
                             }
                             */
                        if (((jennicMsgbuff[1] == 0x1e || jennicMsgbuff[1] == 0x22)) && (jennicMsgbuff[2] == 0))
                        {
                            inp.RtsEnable = true;
                            inp.ReadExisting();
                        }

                        processJennicMsg(jennicMsgbuff);
                    }
                    jennicMsgNextIdx = 0;
                }

            }

        }

        private void buttonConnect_Click(object sender, EventArgs e)
        {
            if (inp.IsOpen)
            {
                inp.Close();
            }
            else
            {
                try
                {
                    inp.PortName = comboBoxPort.Text;
                    inp.Open();
                }
                catch (Exception)
                {
                    // Oops
                    MessageBox.Show("Error opening port", "Port Open", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
            
            
            // Set the button text according to the port state
            if (inp.IsOpen)
            {
                buttonConnect.Text = "Disconnect";
            }
            else
            {
                buttonConnect.Text = "Connect";
            }
        }

        private void SetText(string text)
        {
            // InvokeRequired required compares the thread ID of the
            // calling thread to the thread ID of the creating thread.
            // If these threads are different, it returns true.
            if (this.textBox1.InvokeRequired)
            {
                SetTextCallback d = new SetTextCallback(SetText);
                this.Invoke(d, new object[] { text });
            }
            else
            {
                textBox1.AppendText(text);
            }
        }

        private void SetStatusText(string text)
        {
            if (this.textBox1.InvokeRequired)
            {
                SetTextCallback d = new SetTextCallback(SetStatusText);
                this.Invoke(d, new object[] { text });
            }
            else
            {
                textBoxStatus.Text = text;
            }
        }
        private void buttonProgTX_Click(object sender, EventArgs e)
        {
            string filename = mus.txbinpath;
            string filename5148 = mus.tx5148binpath;
            blupload.upload(filename, filename5148, inp, textBoxStatus, textBox1);


            //    blupload.runFromMem("C:\\Jennic\\cygwin\\jennic\\SDK\\Application\\onewirebootloader\\JN5139_Build\\Release\\onewirebootloader.bin", inp, textBoxStatus, textBox1);
        }

        private void buttonProgRx_Click(object sender, EventArgs e)
        {
            string filename = mus.rxbinpath;
            string filename5148 = mus.rx5148binpath;
            blupload.upload(filename, filename5148, inp, textBoxStatus, textBox1);


            //blupload.upload("C:\\Jennic\\cygwin\\jennic\\SDK\\Application\\onewirebootloader\\JN5139_Build\\Release\\onewirebootloader.bin", inp, textBoxStatus, textBox1);

        }

        private void processJennicMsg(byte[] buff)
        {
            if (this.InvokeRequired)
            {
                jennicMessageCallback d = new jennicMessageCallback(processJennicMsg);
                this.Invoke(d, new object[] { buff });
            }
            else
            {
                byte len = buff[0];
                byte checksum = 0;
                for (int i = 0; i < len; i++)
                {
                    byte b = buff[i];
                    checksum ^= b;
                }
                if (buff[len] != checksum)
                {
                    SetText("@");
                    return;
                }
                byte cmd = buff[1];

                if (cmd == 0x90)
                {
                    //debug message
                    StringBuilder sb = new StringBuilder();
                    for (int i = 2; i < len; i++) sb.Append((char)buff[i]);

                    SetText(sb.ToString());
                }
                else if (cmd == 0x92)
                {
                    lcd1.UpdateBlock((int)buff[2], buff, 3);
                }
                else
                {
                    // StringBuilder sb = new StringBuilder();
                    // for (int i = 0; i <= len; i++) sb.Append((char)buff[i]);
                    //  SetStatusText(sb.ToString());
                }
                if (cmd < 0x30)
                {
                    blupload.processJennicMsg(buff);
                }
                if (cmd == 0xff)
                {
                    logMessage(buff, 0, (byte)buff.Length, ">");
                    pcHandleRoutedMessage(buff, 2, len - 2, 0);
                }
                else
                {
                    if (cmd == 0xfe)
                    {

                    }
                }

                return;
            }
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            // Retrieve the user settings
            mus = new MyUserSettings();
            
            // Create the serial port
            inp = new SerialPort(mus.comPort, 38400, Parity.None, 8, StopBits.One);
             //    inp = new SerialPort(mus.comPort, 250000, Parity.None, 8, StopBits.One);
            
            
            // Hook-up the port data handler
            serEh = new SerialDataReceivedEventHandler(inp_DataReceived);
            inp.DataReceived += serEh;
            
            // Attempt to open the port
            try
            {
                inp.Open();
                inp.RtsEnable = false;
            }
            catch (Exception)
            {
                // Oops
                MessageBox.Show("Error opening port", "Port Open", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }

            // Set the button text according to the port state
            if (inp.IsOpen)
            {
                buttonConnect.Text = "Disconnect";
            }
            else
            {
                buttonConnect.Text = "Connect";
            }
            
            // Initialise the port droplist
            string[] ports = SerialPort.GetPortNames();
            foreach (string port in ports)
            {
                comboBoxPort.Items.Add(port);
            }
            comboBoxPort.SelectedItem = mus.comPort;
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            timer1.Enabled = false;
       //     blupload.timeout();
            if (treeWalker != null)
            {
                routedMessage resp = treeWalker.getNextRequest();
                if (resp != null)
                {
                    timer1.Enabled = true;
                    timer1.Start();
                    SendRoutedMessage(resp, 0);
                }
            }

        }

        private void buttonimageimport_Click(object sender, EventArgs e)
        {
            OpenFileDialog fd = new OpenFileDialog();
            StringBuilder code = new StringBuilder();

            if (fd.ShowDialog(this) == DialogResult.OK)
            {
                code.Append("uint8 " + Path.GetFileNameWithoutExtension(fd.FileName) + "[] = {");
                bool first = true;
                Bitmap img = new Bitmap(fd.FileName);
                for (int y = 0; y < img.Height; y += 8)
                {
                    for (int x = 0; x < img.Width; x++)
                    {
                        byte b = 0;
                        if (img.GetPixel(x, y).GetBrightness() < 0.5) b += 1;
                        if (img.GetPixel(x, y + 1).GetBrightness() < 0.5) b += 2;
                        if (img.GetPixel(x, y + 2).GetBrightness() < 0.5) b += 4;
                        if (img.GetPixel(x, y + 3).GetBrightness() < 0.5) b += 8;
                        if (img.GetPixel(x, y + 4).GetBrightness() < 0.5) b += 16;
                        if (img.GetPixel(x, y + 5).GetBrightness() < 0.5) b += 32;
                        if (img.GetPixel(x, y + 6).GetBrightness() < 0.5) b += 64;
                        if (img.GetPixel(x, y + 7).GetBrightness() < 0.5) b += 128;
                        if (first)
                        {
                            first = false;
                            code.Append("" + b);
                        }
                        else
                        {
                            code.Append("," + b);
                        }
                    }

                    code.Append("\r\n\t");

                }
                code.Append("};");
                img.Dispose();

            }
            Clipboard.SetText(code.ToString());
        }

        private void buttonRefreshTree_Click(object sender, EventArgs e)
        {
            pcNode.children.Clear();
            bindtree(pcNode);
            treeWalker = new routedTreeWalker(pcNode);
            SendRoutedMessage(treeWalker.getNextRequest(), 0);
        }
        private void pcHandleRoutedMessage(byte[] buff, int offset, int len, byte fromCon)
        {
            routedMessage msg = new routedMessage(buff, offset, len);

            //see if packet has reached its destination
            if (msg.Route.isForMe()) //no to addresses
            {
                //see if anything is waiting for this response
                //TODO clear expired subscriptions
                
                foreach (var subscription in pendingResponses)
                {                    
                    if (!subscription.expired && subscription.matches(msg))
                    {
                        subscription.handleReply(msg);
                        if (subscription.singleShot) pendingResponses.Remove(subscription);
                        return;
                    }
                }
                //TODO make everything use subscription and remove the following
                
                
                switch (msg.commandByte)
                {
                    case 0x01: //enumerate response
                        {
                            if (treeWalker != null)
                            {
                                //clear timeout
                                timer1.Stop();
                                timer1.Enabled = false;
                                
                                treeWalker.enumerateResponse(msg);
                                routedMessage resp = treeWalker.getNextRequest();
                                if (resp != null) 
                                {
                                    timer1.Enabled = true;
                                    timer1.Start();
                                    SendRoutedMessage(resp, 0);
                                }
                                else
                                {

                                }
                                bindtree(pcNode);
                            }
                            break;
                        }
                    case 0x91: //code update initialized
                        {
                            if (rUpload != null)
                            {
                                commandReader reader = msg.getReader();
                                reader.ReadByte();
                                string moduleType = reader.ReadString();
                                string filename;
                                if (moduleType == "JN5148")
                                {
                                    if (activeNode.name == "TX")
                                    {
                                        filename = mus.tx5148binpath;
                                    }
                                    else
                                    {
                                        filename = mus.rx5148binpath;
                                    }
                                }
                                else{
                                    if (activeNode.name == "TX")
                                    {
                                        filename = mus.txbinpath;
                                    }
                                    else
                                    {
                                        filename = mus.rxbinpath;
                                    }
                                }
                                rUpload.setFile(filename);
                                // Let the user see the progress
                                ulProg.Show();
                                    
                                routedMessage reply = rUpload.sendNextCmd(msg);
                                if (reply != null) SendRoutedMessage(reply, fromCon);
                            }
                            break;
                        }
                    case 0x93: //code chunk received
                        {
                            if (rUpload != null)
                            {
                                routedMessage reply = rUpload.sendNextCmd(msg);
                                if (reply != null) SendRoutedMessage(reply, fromCon);
                            }
                            break;
                        }
                    case 0x95: // Upload failed
                        {
                            commandReader reader = msg.getReader();
                            reader.ReadByte();
                            string errorMsg;
                            switch (reader.ReadByte()) {
                                case 1:
                                    errorMsg = "CRC Error";
                                    break;
                                case 2:
                                    errorMsg = "Block Length Error";
                                    break;
                                case 3:
                                    errorMsg = "Low Battery";
                                    break;
                                default:
                                    errorMsg = "Unknown Error";
                                    break;
                            }
                            MessageBox.Show("Upload Failed " + errorMsg);
                            break;
                        }
                    case 0xff://debug message
                        {
                            commandReader reader = msg.getReader();
                            reader.ReadByte();
                            SetText(reader.ReadString());
                            break;
                        }
                    case 0x0a://common commands
                        {
                            handleCommonCommand(msg, fromCon);
                            break;
                        }
                    case 0x04://get parameter value response
                        {
                            

                            commandReader reader = msg.getReader();
                            reader.ReadByte();
                            byte paramIdx = reader.ReadByte();
                            //depending on type read value

                            activeNode.getParamByIdx((int)paramIdx).parseValue(msg);

                            propertyGrid1.Refresh();

                            if (activeNode.parameterCount > activeNode.properties.Count)
                            {
                                //get next parameter
                                routedMessage msgGetMeta = new routedMessage(activeNode.address, new byte[] { 0x0a, 0x6, (byte)activeNode.properties.Count });
                                SendRoutedMessage(msgGetMeta, SERIALCON);
                            }
                            break;
                        }
                    case 0x02: // Set parameter response
                        {
                            commandReader reader = msg.getReader();
                            reader.ReadByte();  // Skip Command code
                            bool response = !reader.ReadBoolean();
                            Debug.WriteLine(activeNode.name + ": Parameter set: " + response.ToString());
                            break;
                        }
                    case 0xfe: //loop test
                        {
                            if (loopTest != null)
                            {
                                routedMessage lt = loopTest.sendNextCmd(msg);
                                if (lt != null) SendRoutedMessage(lt, SERIALCON);
                                else
                                {
                                    MessageBox.Show("Loop back done. Length errors " + loopTest.lengtherrors + " content errors " + loopTest.contenterrors);
                                }
                            }
                            break;
                        }
                }
            }
            else
            {
                //todo relay using routedMessage class
                /*
                //relay message
                byte to = buff[offset + addrToIdx + 1];
                //replace to with from
                buff[offset + addrToIdx + 1] = fromCon;
                //move index of next to address
                addrToIdx++;
                buff[offset] = (byte)((buff[offset] & 0xf0) + addrToIdx);
                //pass message on to connector defined by 'to' address
                pcSendRoutedMessage(buff, offset, (byte)len, to);
                */
            }
        }

        //stack of objects waiting for responses

        List<MessageSubscription> pendingResponses=new List<MessageSubscription>();

        public void SendRoutedMessage(routedMessage message, byte toCon, MessageSubscription reply)
        {
            pendingResponses.Add(reply);

            byte[] msg = message.toByteArray();
            pcSendRoutedMessage(msg, 0, (byte)msg.Length, toCon);
        }
        void SendRoutedMessage(routedMessage message, byte toCon)
        {
            byte[] msg = message.toByteArray();
            pcSendRoutedMessage(msg, 0, (byte)msg.Length, toCon);
        }
        void pcSendRoutedMessage(byte[] buff, int offset, byte len, byte toCon)
        {
            logMessage(buff, offset, len,"<");
               
            switch (toCon)
            {
                
                case 0: JennicPacket.Write(inp, buff, offset, len, 0xff); break;
//                case 0: JennicPacket.Write(inp, buff, offset, len, 0xfe); break;
            }
        }
        private void bindtree(routedNode node)
        {
            treeView1.Nodes.Clear();
            bindtree(node, treeView1.Nodes.Add(node.name));
        }
        private void bindtree(routedNode node, TreeNode tNode)
        {
            tNode.Tag = node;
            foreach (routedNode child in node.children.Values)
            {
                TreeNode childt = tNode.Nodes.Add(child.name);
                bindtree(child, childt);
            }
        }

        private void treeView1_AfterSelect(object sender, TreeViewEventArgs e)
        {
            //populate options for node 
            //quick fix use node name to select options
            //todo enumerate options from node or cache
            routedNode node = (routedNode)e.Node.Tag;
            activeNode = node;
            labelNodeName.Text = node.name;
            switch (node.name)
            {
                case "TX":
                    buttonUploadCode.Visible = true;
                    buttonResetNode.Visible = true;
                    getNodeParameters();
                    break;
                case "RX":
                    buttonUploadCode.Visible = true;
                    buttonResetNode.Visible = true;
                    getNodeParameters();
                    break;
                case "PC":
                    buttonUploadCode.Visible = false;
                    buttonResetNode.Visible = false;
                    nodeParameterList.Properties.Clear();
                    activeNode.properties.Clear();
                    clearPropertyViewer();
                    propertyGrid1.Refresh();
                    break;
                default:
                    buttonUploadCode.Visible = false;
                    buttonResetNode.Visible = false;
                    getNodeParameters();
                    break;
            }
        }

        private void buttonUploadCode_Click(object sender, EventArgs e)
        {
            rUpload = new routedUploader();            
            // Instantiate the progress dialog for the uploader
            ulProg = new UploadProgress(rUpload);
            routedMessage msg = new routedMessage(activeNode.address, new byte[] { 0x90 });
            SendRoutedMessage(msg, SERIALCON);
        }

        private void buttonResetNode_Click(object sender, EventArgs e)
        {
            routedMessage msg = new routedMessage(activeNode.address, new byte[] { 0x96 });
            SendRoutedMessage(msg, SERIALCON);
        }
        private void clearPropertyViewer()
        {
            foreach (Button b in commandPanel.Controls)
            {
                b.Click -= cmdButton_Click;
            }
            commandPanel.Controls.Clear();
          
        }

        private void getNodeParameters()
        {
            //nodeParameterList.Properties.Clear();
            activeNode.properties.Clear();

            nodeParameterList.Node=activeNode;
            clearPropertyViewer();
            propertyGrid1.Refresh();

                 
            //send get parameter count command
            routedMessage msg = new routedMessage(activeNode.address, new byte[] { 0x0a, 0x4, 0x00 });
            SendRoutedMessage(msg, SERIALCON);

        }
        private void handleCommonCommand(routedMessage msg, byte fromCon)
        {
            commandReader reader = msg.getReader();
            reader.ReadByte();
           
            byte cmd = reader.ReadByte();
            switch (cmd)
            {
                case 5: //parameter count
                    byte count = reader.ReadByte();
                    activeNode.parameterCount = count;
                    if (count > 0)
                    {
                        routedMessage msgGetMeta = new routedMessage(activeNode.address, new byte[] { 0x0a, 0x6, 0x00 });
                        SendRoutedMessage(msgGetMeta, SERIALCON);
                    }
                    break;
                case 7: //parameter metadata
                    byte pIdx = reader.ReadByte();
                    string pName = reader.ReadString();
                    byte pType = reader.ReadByte();
                    UInt32 pArrayLen = reader.Read7BitEncodedUInt32();

                    try
                    {
                        ccParameter property = new ccParameter(pIdx, pName, pType, pArrayLen, activeNode);
                        activeNode.properties.Add(pName, property);

                        //bind to property editor
                        if (property.TypeIdx != ccParameter.CC_ENUMERATION_VALUES && property.TypeIdx != ccParameter.CC_VOID_FUNCTION)
                        {
                            Flobbster.Windows.Forms.PropertySpec p =
                                new Flobbster.Windows.Forms.PropertySpec(pName, property.type, "Parameters");

                            if (property.TypeIdx == ccParameter.CC_ENUMERATION)
                            {
                                p.ConverterTypeName = "Serial.CC_EnumTypeConverter";
                            }
                            nodeParameterList.Properties.Add(p);

                            propertyGrid1.Refresh();
                        }
                        if (property.TypeIdx == ccParameter.CC_VOID_FUNCTION)
                        {
                            Button cmdButton = new Button();
                            cmdButton.Text = pName;
                            cmdButton.Tag = property;
                            cmdButton.AutoSize = true;
                            cmdButton.Click += new EventHandler(cmdButton_Click);
                            commandPanel.Controls.Add(cmdButton);

                            if (activeNode.parameterCount > activeNode.properties.Count)
                            {
                                //get next parameter
                                routedMessage msgGetMeta = new routedMessage(activeNode.address, new byte[] { 0x0a, 0x6, (byte)activeNode.properties.Count });
                                SendRoutedMessage(msgGetMeta, SERIALCON);
                            }

                        }
                        else
                        {
                            //get value
                            //TODO decide what to do with big arrays - limit to first 32 elements for now

                            routedMessage msgValReq = new routedMessage(activeNode.address, new byte[] { 0x03, pIdx, 0x00, (byte)(Math.Min(pArrayLen,32)) });
                            SendRoutedMessage(msgValReq, SERIALCON);
                        }
                    }
                    catch (Exception pe)
                    {
                        MessageBox.Show("get parameter error" + pe.Message);
                    }
                    break;
            }
        }

        void cmdButton_Click(object sender, EventArgs e)
        {
            //call void function(void) on remote device
            Button b = sender as Button;
            if (b != null)
            {
                ccParameter param = b.Tag as ccParameter;
                if (param != null)
                {
                    byte[] cmd = param.buildSetCmd();

                    if (cmd != null)
                    {
                        routedMessage msgValSet = new routedMessage(param.Owner.address, cmd);
                        SendRoutedMessage(msgValSet, SERIALCON);
                    }
                }
            }
        }
        
        void ExitToolStripMenuItemClick(object sender, EventArgs e)
        {
            Close();
        }
        
        void OptionsToolStripMenuItemClick(object sender, EventArgs e)
        {
            Form optForm = new OptionsForm();
            
            // Display the form modally
            if (DialogResult.OK == optForm.ShowDialog())
            {
                // Check for a changed com port
                if (mus.comPort != inp.PortName)
                {
                    inp.Close();
                    inp.PortName = mus.comPort;
                    buttonConnect.Text = "Connect";
                    if (!comboBoxPort.Items.Contains(mus.comPort))
                    {
                        comboBoxPort.Items.Add(mus.comPort);
                        comboBoxPort.SelectedItem = mus.comPort;
                    }
                }
            }
        }
        
        void Form1FormClosing(object sender, FormClosingEventArgs e)
        {
            if (null != serEh)
            {
                inp.DataReceived -= serEh;
            }
            if (inp.IsOpen)
            {
                inp.Close();
            }
        }

        private void routedComsLoopTestToolStripMenuItem_Click(object sender, EventArgs e)
        {
            //send message to self via tx and rx
           
            loopTest = new loopBackCommsTest(500,new route(new byte[]{48,0,0,1},0));
        
            //loopTest = new loopBackCommsTest(500,new route(new byte[]{16,1},0));
            
        //    loopTest = new loopBackCommsTest(50,new route(new byte[]{16,0},0));
         //   loopTest = new loopBackCommsTest(500, new route(new byte[] { 0 }, 0));
            SendRoutedMessage(loopTest.sendNextCmd(null), SERIALCON);
        }

        private void openScriptToolStripMenuItem_Click(object sender, EventArgs e)
        {
            OpenFileDialog ofd = new OpenFileDialog();
            ofd.Filter = "script files (*.txt)|*.txt|All files (*.*)|*.*";         
            if (ofd.ShowDialog() == DialogResult.OK)
            {
                scriptEditor1.source= File.ReadAllText(ofd.FileName);
            }            
        }

        private void saveScriptToolStripMenuItem_Click(object sender, EventArgs e)
        {
            SaveFileDialog sfd = new SaveFileDialog();
            sfd.Filter = "script files (*.txt)|*.txt|All files (*.*)|*.*";
            if (sfd.ShowDialog() == DialogResult.OK)
            {
                File.WriteAllText(sfd.FileName, scriptEditor1.source);
            }
        }
        private void logMessage(byte[] buff,int offset,byte len,string prefix)
        {
            if (logComms)
            {
                StringBuilder sb = new StringBuilder();
                sb.Append("\r\n"+prefix);
                for (int i = 0; i < len; i++)
                {
                    sb.Append(" " + buff[offset + i].ToString());
                }
                SetText(sb.ToString());
            }
        }
    }
    
}