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
            System.Windows.Forms.MenuStrip menuStrip1;
            System.Windows.Forms.ToolStripMenuItem toolStripMenuItem1;
            System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
            System.Windows.Forms.ToolStripMenuItem toolsToolStripMenuItem;
            System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
            this.exitToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.optionsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.routedComsLoopTestToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
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
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this.propertyGrid1 = new System.Windows.Forms.PropertyGrid();
            this.commandPanel = new System.Windows.Forms.FlowLayoutPanel();
            this.labelNodeName = new System.Windows.Forms.Label();
            this.buttonResetNode = new System.Windows.Forms.Button();
            this.buttonUploadCode = new System.Windows.Forms.Button();
            this.toolStripSeparator3 = new System.Windows.Forms.ToolStripSeparator();
            this.openScriptToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.saveScriptToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.scriptEditor1 = new Serial.scriptEditor();
            this.lcd1 = new Serial.lcd();
            menuStrip1 = new System.Windows.Forms.MenuStrip();
            toolStripMenuItem1 = new System.Windows.Forms.ToolStripMenuItem();
            toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            toolsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            menuStrip1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).BeginInit();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.tableLayoutPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // menuStrip1
            // 
            menuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            toolStripMenuItem1,
            toolsToolStripMenuItem});
            menuStrip1.Location = new System.Drawing.Point(0, 0);
            menuStrip1.Name = "menuStrip1";
            menuStrip1.Padding = new System.Windows.Forms.Padding(8, 2, 0, 2);
            menuStrip1.Size = new System.Drawing.Size(1060, 28);
            menuStrip1.TabIndex = 17;
            menuStrip1.Text = "menuStrip1";
            // 
            // toolStripMenuItem1
            // 
            toolStripMenuItem1.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            toolStripSeparator1,
            this.exitToolStripMenuItem,
            this.toolStripSeparator3,
            this.openScriptToolStripMenuItem,
            this.saveScriptToolStripMenuItem});
            toolStripMenuItem1.Name = "toolStripMenuItem1";
            toolStripMenuItem1.Size = new System.Drawing.Size(44, 24);
            toolStripMenuItem1.Text = "File";
            // 
            // toolStripSeparator1
            // 
            toolStripSeparator1.Name = "toolStripSeparator1";
            toolStripSeparator1.Size = new System.Drawing.Size(153, 6);
            // 
            // exitToolStripMenuItem
            // 
            this.exitToolStripMenuItem.Name = "exitToolStripMenuItem";
            this.exitToolStripMenuItem.Size = new System.Drawing.Size(156, 24);
            this.exitToolStripMenuItem.Text = "Exit";
            this.exitToolStripMenuItem.ToolTipText = "Exit the application";
            this.exitToolStripMenuItem.Click += new System.EventHandler(this.ExitToolStripMenuItemClick);
            // 
            // toolsToolStripMenuItem
            // 
            toolsToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            toolStripSeparator2,
            this.optionsToolStripMenuItem,
            this.routedComsLoopTestToolStripMenuItem});
            toolsToolStripMenuItem.Name = "toolsToolStripMenuItem";
            toolsToolStripMenuItem.Size = new System.Drawing.Size(57, 24);
            toolsToolStripMenuItem.Text = "Tools";
            // 
            // toolStripSeparator2
            // 
            toolStripSeparator2.Name = "toolStripSeparator2";
            toolStripSeparator2.Size = new System.Drawing.Size(221, 6);
            // 
            // optionsToolStripMenuItem
            // 
            this.optionsToolStripMenuItem.Name = "optionsToolStripMenuItem";
            this.optionsToolStripMenuItem.Size = new System.Drawing.Size(224, 24);
            this.optionsToolStripMenuItem.Text = "Options...";
            this.optionsToolStripMenuItem.Click += new System.EventHandler(this.OptionsToolStripMenuItemClick);
            // 
            // routedComsLoopTestToolStripMenuItem
            // 
            this.routedComsLoopTestToolStripMenuItem.Name = "routedComsLoopTestToolStripMenuItem";
            this.routedComsLoopTestToolStripMenuItem.Size = new System.Drawing.Size(224, 24);
            this.routedComsLoopTestToolStripMenuItem.Text = "routed coms loop test";
            this.routedComsLoopTestToolStripMenuItem.Click += new System.EventHandler(this.routedComsLoopTestToolStripMenuItem_Click);
            // 
            // textBox1
            // 
            this.textBox1.Location = new System.Drawing.Point(12, 624);
            this.textBox1.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.textBox1.Multiline = true;
            this.textBox1.Name = "textBox1";
            this.textBox1.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.textBox1.Size = new System.Drawing.Size(603, 142);
            this.textBox1.TabIndex = 0;
            // 
            // buttonConnect
            // 
            this.buttonConnect.Location = new System.Drawing.Point(12, 53);
            this.buttonConnect.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.buttonConnect.Name = "buttonConnect";
            this.buttonConnect.Size = new System.Drawing.Size(111, 25);
            this.buttonConnect.TabIndex = 1;
            this.buttonConnect.Text = "Connect";
            this.buttonConnect.UseVisualStyleBackColor = true;
            this.buttonConnect.Click += new System.EventHandler(this.buttonConnect_Click);
            // 
            // buttonProgTx
            // 
            this.buttonProgTx.Location = new System.Drawing.Point(12, 84);
            this.buttonProgTx.Margin = new System.Windows.Forms.Padding(4);
            this.buttonProgTx.Name = "buttonProgTx";
            this.buttonProgTx.Size = new System.Drawing.Size(220, 28);
            this.buttonProgTx.TabIndex = 2;
            this.buttonProgTx.Text = "Boot Load Prog Tx";
            this.buttonProgTx.UseVisualStyleBackColor = true;
            this.buttonProgTx.Click += new System.EventHandler(this.buttonProgTX_Click);
            // 
            // buttonProgRx
            // 
            this.buttonProgRx.Location = new System.Drawing.Point(12, 119);
            this.buttonProgRx.Margin = new System.Windows.Forms.Padding(4);
            this.buttonProgRx.Name = "buttonProgRx";
            this.buttonProgRx.Size = new System.Drawing.Size(220, 28);
            this.buttonProgRx.TabIndex = 3;
            this.buttonProgRx.Text = "Boot Load Prog Rx";
            this.buttonProgRx.UseVisualStyleBackColor = true;
            this.buttonProgRx.Click += new System.EventHandler(this.buttonProgRx_Click);
            // 
            // textBoxStatus
            // 
            this.textBoxStatus.Location = new System.Drawing.Point(12, 597);
            this.textBoxStatus.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.textBoxStatus.Name = "textBoxStatus";
            this.textBoxStatus.ReadOnly = true;
            this.textBoxStatus.Size = new System.Drawing.Size(603, 22);
            this.textBoxStatus.TabIndex = 5;
            // 
            // comboBoxPort
            // 
            this.comboBoxPort.FormattingEnabled = true;
            this.comboBoxPort.Location = new System.Drawing.Point(133, 54);
            this.comboBoxPort.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.comboBoxPort.Name = "comboBoxPort";
            this.comboBoxPort.Size = new System.Drawing.Size(97, 24);
            this.comboBoxPort.Sorted = true;
            this.comboBoxPort.TabIndex = 7;
            // 
            // timer1
            // 
            this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
            // 
            // buttonimageimport
            // 
            this.buttonimageimport.Location = new System.Drawing.Point(15, 153);
            this.buttonimageimport.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.buttonimageimport.Name = "buttonimageimport";
            this.buttonimageimport.Size = new System.Drawing.Size(220, 26);
            this.buttonimageimport.TabIndex = 9;
            this.buttonimageimport.Text = "Import Image";
            this.buttonimageimport.UseVisualStyleBackColor = true;
            this.buttonimageimport.Click += new System.EventHandler(this.buttonimageimport_Click);
            // 
            // buttonrefreshTree
            // 
            this.buttonrefreshTree.Location = new System.Drawing.Point(12, 186);
            this.buttonrefreshTree.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.buttonrefreshTree.Name = "buttonrefreshTree";
            this.buttonrefreshTree.Size = new System.Drawing.Size(192, 23);
            this.buttonrefreshTree.TabIndex = 12;
            this.buttonrefreshTree.Text = "Refresh Tree";
            this.buttonrefreshTree.UseVisualStyleBackColor = true;
            this.buttonrefreshTree.Click += new System.EventHandler(this.buttonRefreshTree_Click);
            // 
            // splitContainer1
            // 
            this.splitContainer1.Location = new System.Drawing.Point(12, 213);
            this.splitContainer1.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.splitContainer1.Name = "splitContainer1";
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.treeView1);
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.BackColor = System.Drawing.SystemColors.ControlLightLight;
            this.splitContainer1.Panel2.Controls.Add(this.tableLayoutPanel1);
            this.splitContainer1.Panel2.Controls.Add(this.labelNodeName);
            this.splitContainer1.Panel2.Controls.Add(this.buttonResetNode);
            this.splitContainer1.Panel2.Controls.Add(this.buttonUploadCode);
            this.splitContainer1.Size = new System.Drawing.Size(599, 366);
            this.splitContainer1.SplitterDistance = 198;
            this.splitContainer1.TabIndex = 16;
            // 
            // treeView1
            // 
            this.treeView1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.treeView1.Location = new System.Drawing.Point(0, 0);
            this.treeView1.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.treeView1.Name = "treeView1";
            this.treeView1.Size = new System.Drawing.Size(198, 366);
            this.treeView1.TabIndex = 0;
            this.treeView1.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.treeView1_AfterSelect);
            // 
            // tableLayoutPanel1
            // 
            this.tableLayoutPanel1.AutoSize = true;
            this.tableLayoutPanel1.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.tableLayoutPanel1.ColumnCount = 1;
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel1.Controls.Add(this.propertyGrid1, 0, 1);
            this.tableLayoutPanel1.Controls.Add(this.commandPanel, 0, 0);
            this.tableLayoutPanel1.Location = new System.Drawing.Point(3, 42);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            this.tableLayoutPanel1.RowCount = 2;
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel1.Size = new System.Drawing.Size(386, 324);
            this.tableLayoutPanel1.TabIndex = 3;
            // 
            // propertyGrid1
            // 
            this.propertyGrid1.Location = new System.Drawing.Point(3, 8);
            this.propertyGrid1.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.propertyGrid1.Name = "propertyGrid1";
            this.propertyGrid1.Size = new System.Drawing.Size(380, 314);
            this.propertyGrid1.TabIndex = 4;
            // 
            // commandPanel
            // 
            this.commandPanel.AutoSize = true;
            this.commandPanel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.commandPanel.Location = new System.Drawing.Point(3, 3);
            this.commandPanel.Name = "commandPanel";
            this.commandPanel.Size = new System.Drawing.Size(0, 0);
            this.commandPanel.TabIndex = 5;
            // 
            // labelNodeName
            // 
            this.labelNodeName.AutoSize = true;
            this.labelNodeName.Location = new System.Drawing.Point(13, 14);
            this.labelNodeName.Name = "labelNodeName";
            this.labelNodeName.Size = new System.Drawing.Size(26, 17);
            this.labelNodeName.TabIndex = 2;
            this.labelNodeName.Text = "PC";
            // 
            // buttonResetNode
            // 
            this.buttonResetNode.Location = new System.Drawing.Point(261, 14);
            this.buttonResetNode.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.buttonResetNode.Name = "buttonResetNode";
            this.buttonResetNode.Size = new System.Drawing.Size(104, 23);
            this.buttonResetNode.TabIndex = 1;
            this.buttonResetNode.Text = "Reset Node";
            this.buttonResetNode.UseVisualStyleBackColor = true;
            this.buttonResetNode.Visible = false;
            this.buttonResetNode.Click += new System.EventHandler(this.buttonResetNode_Click);
            // 
            // buttonUploadCode
            // 
            this.buttonUploadCode.Location = new System.Drawing.Point(147, 14);
            this.buttonUploadCode.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.buttonUploadCode.Name = "buttonUploadCode";
            this.buttonUploadCode.Size = new System.Drawing.Size(109, 23);
            this.buttonUploadCode.TabIndex = 0;
            this.buttonUploadCode.Text = "Update Code";
            this.buttonUploadCode.UseVisualStyleBackColor = true;
            this.buttonUploadCode.Visible = false;
            this.buttonUploadCode.Click += new System.EventHandler(this.buttonUploadCode_Click);
            // 
            // toolStripSeparator3
            // 
            this.toolStripSeparator3.Name = "toolStripSeparator3";
            this.toolStripSeparator3.Size = new System.Drawing.Size(153, 6);
            // 
            // openScriptToolStripMenuItem
            // 
            this.openScriptToolStripMenuItem.Name = "openScriptToolStripMenuItem";
            this.openScriptToolStripMenuItem.Size = new System.Drawing.Size(156, 24);
            this.openScriptToolStripMenuItem.Text = "Open Script";
            this.openScriptToolStripMenuItem.Click += new System.EventHandler(this.openScriptToolStripMenuItem_Click);
            // 
            // saveScriptToolStripMenuItem
            // 
            this.saveScriptToolStripMenuItem.Name = "saveScriptToolStripMenuItem";
            this.saveScriptToolStripMenuItem.Size = new System.Drawing.Size(156, 24);
            this.saveScriptToolStripMenuItem.Text = "Save Script";
            this.saveScriptToolStripMenuItem.Click += new System.EventHandler(this.saveScriptToolStripMenuItem_Click);
            // 
            // scriptEditor1
            // 
            this.scriptEditor1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.scriptEditor1.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.scriptEditor1.Location = new System.Drawing.Point(621, 28);
            this.scriptEditor1.Name = "scriptEditor1";
            this.scriptEditor1.PC = null;
            this.scriptEditor1.Size = new System.Drawing.Size(439, 753);
            this.scriptEditor1.source = "";
            this.scriptEditor1.TabIndex = 18;
            // 
            // lcd1
            // 
            this.lcd1.Location = new System.Drawing.Point(266, 39);
            this.lcd1.Margin = new System.Windows.Forms.Padding(5);
            this.lcd1.Name = "lcd1";
            this.lcd1.Size = new System.Drawing.Size(341, 158);
            this.lcd1.TabIndex = 6;
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 16F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(1060, 781);
            this.Controls.Add(this.scriptEditor1);
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
            this.Controls.Add(menuStrip1);
            this.MainMenuStrip = menuStrip1;
            this.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.Name = "Form1";
            this.Text = "RC24 Configuation";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.Form1FormClosing);
            this.Load += new System.EventHandler(this.Form1_Load);
            menuStrip1.ResumeLayout(false);
            menuStrip1.PerformLayout();
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel2.ResumeLayout(false);
            this.splitContainer1.Panel2.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).EndInit();
            this.splitContainer1.ResumeLayout(false);
            this.tableLayoutPanel1.ResumeLayout(false);
            this.tableLayoutPanel1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }
        private System.Windows.Forms.ToolStripMenuItem optionsToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem exitToolStripMenuItem;

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
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
        private System.Windows.Forms.FlowLayoutPanel commandPanel;
        private System.Windows.Forms.ToolStripMenuItem routedComsLoopTestToolStripMenuItem;
        private scriptEditor scriptEditor1;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator3;
        private System.Windows.Forms.ToolStripMenuItem openScriptToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem saveScriptToolStripMenuItem;
    }
}

