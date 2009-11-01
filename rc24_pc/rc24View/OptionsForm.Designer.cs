/*
 * Created by SharpDevelop.
 * User: Graham
 * Date: 31/10/2009
 * Time: 13:11
 * 
 * To change this template use Tools | Options | Coding | Edit Standard Headers.
 */
namespace Serial
{
    partial class OptionsForm
    {
        /// <summary>
        /// Designer variable used to keep track of non-visual components.
        /// </summary>
        private System.ComponentModel.IContainer components = null;
        
        /// <summary>
        /// Disposes resources used by the form.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing) {
                if (components != null) {
                    components.Dispose();
                }
            }
            base.Dispose(disposing);
        }
        
        /// <summary>
        /// This method is required for Windows Forms designer support.
        /// Do not change the method contents inside the source code editor. The Forms designer might
        /// not be able to load this method if it was changed manually.
        /// </summary>
        private void InitializeComponent()
        {
        	System.Windows.Forms.Label label1;
        	System.Windows.Forms.Label label2;
        	System.Windows.Forms.GroupBox groupBox1;
        	System.Windows.Forms.Button optRxPathButton;
        	System.Windows.Forms.Button optTxPathButton;
        	System.Windows.Forms.Label label3;
        	System.Windows.Forms.GroupBox groupBox2;
        	System.Windows.Forms.Label label4;
        	System.Windows.Forms.Button optRx5148PathButton;
        	System.Windows.Forms.Button optTx5148PathButton;
        	System.Windows.Forms.Label label5;
        	this.txBinPathTextBox = new System.Windows.Forms.TextBox();
        	this.rxBinPathTextBox = new System.Windows.Forms.TextBox();
        	this.tx5148BinPathTextBox = new System.Windows.Forms.TextBox();
        	this.rx5148BinPathTextBox = new System.Windows.Forms.TextBox();
        	this.optOKButton = new System.Windows.Forms.Button();
        	this.optCancelButton = new System.Windows.Forms.Button();
        	this.comPortUpDown = new System.Windows.Forms.NumericUpDown();
        	label1 = new System.Windows.Forms.Label();
        	label2 = new System.Windows.Forms.Label();
        	groupBox1 = new System.Windows.Forms.GroupBox();
        	optRxPathButton = new System.Windows.Forms.Button();
        	optTxPathButton = new System.Windows.Forms.Button();
        	label3 = new System.Windows.Forms.Label();
        	groupBox2 = new System.Windows.Forms.GroupBox();
        	label4 = new System.Windows.Forms.Label();
        	optRx5148PathButton = new System.Windows.Forms.Button();
        	optTx5148PathButton = new System.Windows.Forms.Button();
        	label5 = new System.Windows.Forms.Label();
        	groupBox1.SuspendLayout();
        	groupBox2.SuspendLayout();
        	((System.ComponentModel.ISupportInitialize)(this.comPortUpDown)).BeginInit();
        	this.SuspendLayout();
        	// 
        	// label1
        	// 
        	label1.AutoSize = true;
        	label1.Location = new System.Drawing.Point(6, 16);
        	label1.Name = "label1";
        	label1.Size = new System.Drawing.Size(54, 13);
        	label1.TabIndex = 1;
        	label1.Text = "Tx Binary:";
        	// 
        	// label2
        	// 
        	label2.AutoSize = true;
        	label2.Location = new System.Drawing.Point(6, 65);
        	label2.Name = "label2";
        	label2.Size = new System.Drawing.Size(55, 13);
        	label2.TabIndex = 2;
        	label2.Text = "Rx Binary:";
        	// 
        	// groupBox1
        	// 
        	groupBox1.Controls.Add(label1);
        	groupBox1.Controls.Add(optRxPathButton);
        	groupBox1.Controls.Add(this.txBinPathTextBox);
        	groupBox1.Controls.Add(optTxPathButton);
        	groupBox1.Controls.Add(label2);
        	groupBox1.Controls.Add(this.rxBinPathTextBox);
        	groupBox1.Location = new System.Drawing.Point(12, 12);
        	groupBox1.Name = "groupBox1";
        	groupBox1.Size = new System.Drawing.Size(268, 112);
        	groupBox1.TabIndex = 2;
        	groupBox1.TabStop = false;
        	groupBox1.Text = "Binary Files";
        	// 
        	// optRxPathButton
        	// 
        	optRxPathButton.AutoSize = true;
        	optRxPathButton.Location = new System.Drawing.Point(236, 78);
        	optRxPathButton.Name = "optRxPathButton";
        	optRxPathButton.Size = new System.Drawing.Size(26, 23);
        	optRxPathButton.TabIndex = 3;
        	optRxPathButton.Text = "...";
        	optRxPathButton.UseVisualStyleBackColor = true;
        	optRxPathButton.Click += new System.EventHandler(this.OptRxPathButtonClick);
        	// 
        	// txBinPathTextBox
        	// 
        	this.txBinPathTextBox.Location = new System.Drawing.Point(6, 31);
        	this.txBinPathTextBox.Name = "txBinPathTextBox";
        	this.txBinPathTextBox.Size = new System.Drawing.Size(224, 20);
        	this.txBinPathTextBox.TabIndex = 0;
        	// 
        	// optTxPathButton
        	// 
        	optTxPathButton.AutoSize = true;
        	optTxPathButton.Location = new System.Drawing.Point(236, 29);
        	optTxPathButton.Name = "optTxPathButton";
        	optTxPathButton.Size = new System.Drawing.Size(26, 23);
        	optTxPathButton.TabIndex = 1;
        	optTxPathButton.Text = "...";
        	optTxPathButton.UseVisualStyleBackColor = true;
        	optTxPathButton.Click += new System.EventHandler(this.OptTxPathButtonClick);
        	// 
        	// rxBinPathTextBox
        	// 
        	this.rxBinPathTextBox.Location = new System.Drawing.Point(6, 80);
        	this.rxBinPathTextBox.Name = "rxBinPathTextBox";
        	this.rxBinPathTextBox.Size = new System.Drawing.Size(224, 20);
        	this.rxBinPathTextBox.TabIndex = 2;
        	// 
        	// label3
        	// 
        	label3.AutoSize = true;
        	label3.Location = new System.Drawing.Point(12, 254);
        	label3.Name = "label3";
        	label3.Size = new System.Drawing.Size(53, 13);
        	label3.TabIndex = 10;
        	label3.Text = "Com Port:";
        	// 
        	// groupBox2
        	// 
        	groupBox2.Controls.Add(label4);
        	groupBox2.Controls.Add(optRx5148PathButton);
        	groupBox2.Controls.Add(this.tx5148BinPathTextBox);
        	groupBox2.Controls.Add(optTx5148PathButton);
        	groupBox2.Controls.Add(label5);
        	groupBox2.Controls.Add(this.rx5148BinPathTextBox);
        	groupBox2.Location = new System.Drawing.Point(12, 130);
        	groupBox2.Name = "groupBox2";
        	groupBox2.Size = new System.Drawing.Size(268, 112);
        	groupBox2.TabIndex = 3;
        	groupBox2.TabStop = false;
        	groupBox2.Text = "5148 Binary Files";
        	// 
        	// label4
        	// 
        	label4.AutoSize = true;
        	label4.Location = new System.Drawing.Point(6, 16);
        	label4.Name = "label4";
        	label4.Size = new System.Drawing.Size(54, 13);
        	label4.TabIndex = 1;
        	label4.Text = "Tx Binary:";
        	// 
        	// optRx5148PathButton
        	// 
        	optRx5148PathButton.AutoSize = true;
        	optRx5148PathButton.Location = new System.Drawing.Point(236, 78);
        	optRx5148PathButton.Name = "optRx5148PathButton";
        	optRx5148PathButton.Size = new System.Drawing.Size(26, 23);
        	optRx5148PathButton.TabIndex = 2;
        	optRx5148PathButton.Text = "...";
        	optRx5148PathButton.UseVisualStyleBackColor = true;
        	optRx5148PathButton.Click += new System.EventHandler(this.OptRx5148PathButtonClick);
        	// 
        	// tx5148BinPathTextBox
        	// 
        	this.tx5148BinPathTextBox.Location = new System.Drawing.Point(6, 31);
        	this.tx5148BinPathTextBox.Name = "tx5148BinPathTextBox";
        	this.tx5148BinPathTextBox.Size = new System.Drawing.Size(224, 20);
        	this.tx5148BinPathTextBox.TabIndex = 0;
        	// 
        	// optTx5148PathButton
        	// 
        	optTx5148PathButton.AutoSize = true;
        	optTx5148PathButton.Location = new System.Drawing.Point(236, 29);
        	optTx5148PathButton.Name = "optTx5148PathButton";
        	optTx5148PathButton.Size = new System.Drawing.Size(26, 23);
        	optTx5148PathButton.TabIndex = 0;
        	optTx5148PathButton.Text = "...";
        	optTx5148PathButton.UseVisualStyleBackColor = true;
        	optTx5148PathButton.Click += new System.EventHandler(this.OptTx5148PathButtonClick);
        	// 
        	// label5
        	// 
        	label5.AutoSize = true;
        	label5.Location = new System.Drawing.Point(6, 65);
        	label5.Name = "label5";
        	label5.Size = new System.Drawing.Size(55, 13);
        	label5.TabIndex = 2;
        	label5.Text = "Rx Binary:";
        	// 
        	// rx5148BinPathTextBox
        	// 
        	this.rx5148BinPathTextBox.Location = new System.Drawing.Point(6, 80);
        	this.rx5148BinPathTextBox.Name = "rx5148BinPathTextBox";
        	this.rx5148BinPathTextBox.Size = new System.Drawing.Size(224, 20);
        	this.rx5148BinPathTextBox.TabIndex = 1;
        	// 
        	// optOKButton
        	// 
        	this.optOKButton.DialogResult = System.Windows.Forms.DialogResult.OK;
        	this.optOKButton.Location = new System.Drawing.Point(123, 289);
        	this.optOKButton.Name = "optOKButton";
        	this.optOKButton.Size = new System.Drawing.Size(75, 23);
        	this.optOKButton.TabIndex = 1;
        	this.optOKButton.Text = "OK";
        	this.optOKButton.UseVisualStyleBackColor = true;
        	this.optOKButton.Click += new System.EventHandler(this.OptOKButtonClick);
        	// 
        	// optCancelButton
        	// 
        	this.optCancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
        	this.optCancelButton.Location = new System.Drawing.Point(205, 289);
        	this.optCancelButton.Name = "optCancelButton";
        	this.optCancelButton.Size = new System.Drawing.Size(75, 23);
        	this.optCancelButton.TabIndex = 0;
        	this.optCancelButton.Text = "Cancel";
        	this.optCancelButton.UseVisualStyleBackColor = true;
        	this.optCancelButton.Click += new System.EventHandler(this.CancelButtonClick);
        	// 
        	// comPortUpDown
        	// 
        	this.comPortUpDown.Location = new System.Drawing.Point(71, 251);
        	this.comPortUpDown.Maximum = new decimal(new int[] {
        	        	        	9,
        	        	        	0,
        	        	        	0,
        	        	        	0});
        	this.comPortUpDown.Minimum = new decimal(new int[] {
        	        	        	1,
        	        	        	0,
        	        	        	0,
        	        	        	0});
        	this.comPortUpDown.Name = "comPortUpDown";
        	this.comPortUpDown.Size = new System.Drawing.Size(37, 20);
        	this.comPortUpDown.TabIndex = 4;
        	this.comPortUpDown.Value = new decimal(new int[] {
        	        	        	1,
        	        	        	0,
        	        	        	0,
        	        	        	0});
        	// 
        	// OptionsForm
        	// 
        	this.AcceptButton = this.optOKButton;
        	this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
        	this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
        	this.CancelButton = this.optCancelButton;
        	this.ClientSize = new System.Drawing.Size(292, 324);
        	this.Controls.Add(groupBox2);
        	this.Controls.Add(this.comPortUpDown);
        	this.Controls.Add(label3);
        	this.Controls.Add(groupBox1);
        	this.Controls.Add(this.optCancelButton);
        	this.Controls.Add(this.optOKButton);
        	this.MaximizeBox = false;
        	this.MinimizeBox = false;
        	this.Name = "OptionsForm";
        	this.Text = "Options";
        	this.TopMost = true;
        	this.Load += new System.EventHandler(this.OptionsFormLoad);
        	groupBox1.ResumeLayout(false);
        	groupBox1.PerformLayout();
        	groupBox2.ResumeLayout(false);
        	groupBox2.PerformLayout();
        	((System.ComponentModel.ISupportInitialize)(this.comPortUpDown)).EndInit();
        	this.ResumeLayout(false);
        	this.PerformLayout();
        }
        private System.Windows.Forms.TextBox rx5148BinPathTextBox;
        private System.Windows.Forms.TextBox tx5148BinPathTextBox;
        private System.Windows.Forms.NumericUpDown comPortUpDown;
        private System.Windows.Forms.Button optOKButton;
        private System.Windows.Forms.Button optCancelButton;
        private System.Windows.Forms.TextBox rxBinPathTextBox;
        private System.Windows.Forms.TextBox txBinPathTextBox;
    }
}
