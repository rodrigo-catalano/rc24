namespace Serial
{
    partial class Form1
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.textBox1 = new System.Windows.Forms.TextBox();
            this.buttonConnect = new System.Windows.Forms.Button();
            this.buttonProgTx = new System.Windows.Forms.Button();
            this.buttonProgRx = new System.Windows.Forms.Button();
            this.textBoxStatus = new System.Windows.Forms.TextBox();
            this.comboBoxPort = new System.Windows.Forms.ComboBox();
            this.timer1 = new System.Windows.Forms.Timer(this.components);
            this.buttonimageimport = new System.Windows.Forms.Button();
            this.buttonrefreshTree = new System.Windows.Forms.Button();
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.treeView1 = new System.Windows.Forms.TreeView();
            this.propertyGrid1 = new System.Windows.Forms.PropertyGrid();
            this.labelNodeName = new System.Windows.Forms.Label();
            this.buttonResetNode = new System.Windows.Forms.Button();
            this.buttonUploadCode = new System.Windows.Forms.Button();
            this.lcd1 = new Serial.lcd();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.SuspendLayout();
            // 
            // textBox1
            // 
            this.textBox1.Location = new System.Drawing.Point(9, 507);
            this.textBox1.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
            this.textBox1.Multiline = true;
            this.textBox1.Name = "textBox1";
            this.textBox1.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.textBox1.Size = new System.Drawing.Size(453, 116);
            this.textBox1.TabIndex = 0;
            // 
            // buttonConnect
            // 
            this.buttonConnect.Location = new System.Drawing.Point(9, 10);
            this.buttonConnect.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
            this.buttonConnect.Name = "buttonConnect";
            this.buttonConnect.Size = new System.Drawing.Size(67, 20);
            this.buttonConnect.TabIndex = 1;
            this.buttonConnect.Text = "Connect";
            this.buttonConnect.UseVisualStyleBackColor = true;
            this.buttonConnect.Click += new System.EventHandler(this.buttonConnect_Click);
            // 
            // buttonProgTx
            // 
            this.buttonProgTx.Location = new System.Drawing.Point(9, 35);
            this.buttonProgTx.Name = "buttonProgTx";
            this.buttonProgTx.Size = new System.Drawing.Size(118, 23);
            this.buttonProgTx.TabIndex = 2;
            this.buttonProgTx.Text = "Boot Load Prog Tx";
            this.buttonProgTx.UseVisualStyleBackColor = true;
            this.buttonProgTx.Click += new System.EventHandler(this.buttonProgTX_Click);
            // 
            // buttonProgRx
            // 
            this.buttonProgRx.Location = new System.Drawing.Point(9, 64);
            this.buttonProgRx.Name = "buttonProgRx";
            this.buttonProgRx.Size = new System.Drawing.Size(118, 23);
            this.buttonProgRx.TabIndex = 3;
            this.buttonProgRx.Text = "Boot Load Prog Rx";
            this.buttonProgRx.UseVisualStyleBackColor = true;
            this.buttonProgRx.Click += new System.EventHandler(this.buttonProgRx_Click);
            // 
            // textBoxStatus
            // 
            this.textBoxStatus.Location = new System.Drawing.Point(9, 485);
            this.textBoxStatus.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
            this.textBoxStatus.Name = "textBoxStatus";
            this.textBoxStatus.ReadOnly = true;
            this.textBoxStatus.Size = new System.Drawing.Size(453, 20);
            this.textBoxStatus.TabIndex = 5;
            // 
            // comboBoxPort
            // 
            this.comboBoxPort.FormattingEnabled = true;
            this.comboBoxPort.Location = new System.Drawing.Point(80, 10);
            this.comboBoxPort.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
            this.comboBoxPort.Name = "comboBoxPort";
            this.comboBoxPort.Size = new System.Drawing.Size(48, 21);
            this.comboBoxPort.TabIndex = 7;
            // 
            // timer1
            // 
            this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
            // 
            // buttonimageimport
            // 
            this.buttonimageimport.Location = new System.Drawing.Point(9, 93);
            this.buttonimageimport.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
            this.buttonimageimport.Name = "buttonimageimport";
            this.buttonimageimport.Size = new System.Drawing.Size(118, 21);
            this.buttonimageimport.TabIndex = 9;
            this.buttonimageimport.Text = "Import Image";
            this.buttonimageimport.UseVisualStyleBackColor = true;
            this.buttonimageimport.Click += new System.EventHandler(this.buttonimageimport_Click);
            // 
            // buttonrefreshTree
            // 
            this.buttonrefreshTree.Location = new System.Drawing.Point(9, 151);
            this.buttonrefreshTree.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
            this.buttonrefreshTree.Name = "buttonrefreshTree";
            this.buttonrefreshTree.Size = new System.Drawing.Size(144, 19);
            this.buttonrefreshTree.TabIndex = 12;
            this.buttonrefreshTree.Text = "Refresh Tree";
            this.buttonrefreshTree.UseVisualStyleBackColor = true;
            this.buttonrefreshTree.Click += new System.EventHandler(this.buttonRefreshTree_Click);
            // 
            // splitContainer1
            // 
            this.splitContainer1.Location = new System.Drawing.Point(10, 184);
            this.splitContainer1.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
            this.splitContainer1.Name = "splitContainer1";
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.treeView1);
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.BackColor = System.Drawing.SystemColors.ControlLightLight;
            this.splitContainer1.Panel2.Controls.Add(this.propertyGrid1);
            this.splitContainer1.Panel2.Controls.Add(this.labelNodeName);
            this.splitContainer1.Panel2.Controls.Add(this.buttonResetNode);
            this.splitContainer1.Panel2.Controls.Add(this.buttonUploadCode);
            this.splitContainer1.Size = new System.Drawing.Size(449, 297);
            this.splitContainer1.SplitterDistance = 149;
            this.splitContainer1.SplitterWidth = 3;
            this.splitContainer1.TabIndex = 16;
            // 
            // treeView1
            // 
            this.treeView1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.treeView1.Location = new System.Drawing.Point(0, 0);
            this.treeView1.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
            this.treeView1.Name = "treeView1";
            this.treeView1.Size = new System.Drawing.Size(149, 297);
            this.treeView1.TabIndex = 0;
            this.treeView1.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.treeView1_AfterSelect);
            // 
            // propertyGrid1
            // 
            this.propertyGrid1.Location = new System.Drawing.Point(2, 37);
            this.propertyGrid1.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
            this.propertyGrid1.Name = "propertyGrid1";
            this.propertyGrid1.Size = new System.Drawing.Size(292, 259);
            this.propertyGrid1.TabIndex = 4;
            // 
            // labelNodeName
            // 
            this.labelNodeName.AutoSize = true;
            this.labelNodeName.Location = new System.Drawing.Point(10, 11);
            this.labelNodeName.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
            this.labelNodeName.Name = "labelNodeName";
            this.labelNodeName.Size = new System.Drawing.Size(21, 13);
            this.labelNodeName.TabIndex = 2;
            this.labelNodeName.Text = "PC";
            // 
            // buttonResetNode
            // 
            this.buttonResetNode.Location = new System.Drawing.Point(196, 11);
            this.buttonResetNode.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
            this.buttonResetNode.Name = "buttonResetNode";
            this.buttonResetNode.Size = new System.Drawing.Size(78, 19);
            this.buttonResetNode.TabIndex = 1;
            this.buttonResetNode.Text = "Reset Node";
            this.buttonResetNode.UseVisualStyleBackColor = true;
            this.buttonResetNode.Visible = false;
            this.buttonResetNode.Click += new System.EventHandler(this.buttonResetNode_Click);
            // 
            // buttonUploadCode
            // 
            this.buttonUploadCode.Location = new System.Drawing.Point(110, 11);
            this.buttonUploadCode.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
            this.buttonUploadCode.Name = "buttonUploadCode";
            this.buttonUploadCode.Size = new System.Drawing.Size(82, 19);
            this.buttonUploadCode.TabIndex = 0;
            this.buttonUploadCode.Text = "Update Code";
            this.buttonUploadCode.UseVisualStyleBackColor = true;
            this.buttonUploadCode.Visible = false;
            this.buttonUploadCode.Click += new System.EventHandler(this.buttonUploadCode_Click);
            // 
            // lcd1
            // 
            this.lcd1.Location = new System.Drawing.Point(196, 18);
            this.lcd1.Name = "lcd1";
            this.lcd1.Size = new System.Drawing.Size(256, 128);
            this.lcd1.TabIndex = 6;
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(468, 631);
            this.Controls.Add(this.splitContainer1);
            this.Controls.Add(this.buttonrefreshTree);
            this.Controls.Add(this.buttonimageimport);
            this.Controls.Add(this.comboBoxPort);
            this.Controls.Add(this.lcd1);
            this.Controls.Add(this.textBoxStatus);
            this.Controls.Add(this.buttonProgRx);
            this.Controls.Add(this.buttonProgTx);
            this.Controls.Add(this.buttonConnect);
            this.Controls.Add(this.textBox1);
            this.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
            this.Name = "Form1";
            this.Text = "RC24 Configuation";
            this.Load += new System.EventHandler(this.Form1_Load);
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel2.ResumeLayout(false);
            this.splitContainer1.Panel2.PerformLayout();
            this.splitContainer1.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TextBox textBox1;
        private System.Windows.Forms.Button buttonConnect;
        private System.Windows.Forms.Button buttonProgTx;
        private System.Windows.Forms.Button buttonProgRx;
        private System.Windows.Forms.TextBox textBoxStatus;
        private lcd lcd1;
        private System.Windows.Forms.ComboBox comboBoxPort;
        private System.Windows.Forms.Timer timer1;
        private System.Windows.Forms.Button buttonimageimport;
        private System.Windows.Forms.Button buttonrefreshTree;
        private System.Windows.Forms.SplitContainer splitContainer1;
        private System.Windows.Forms.TreeView treeView1;
        private System.Windows.Forms.Label labelNodeName;
        private System.Windows.Forms.Button buttonResetNode;
        private System.Windows.Forms.Button buttonUploadCode;
        private System.Windows.Forms.PropertyGrid propertyGrid1;
    }
}

