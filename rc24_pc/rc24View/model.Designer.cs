namespace Serial
{
    partial class model
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
            this.rate = new System.Windows.Forms.NumericUpDown();
            this.expo = new System.Windows.Forms.NumericUpDown();
            this.graph1 = new Serial.graph();
            ((System.ComponentModel.ISupportInitialize)(this.rate)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.expo)).BeginInit();
            this.SuspendLayout();
            // 
            // rate
            // 
            this.rate.Location = new System.Drawing.Point(100, 23);
            this.rate.Minimum = new decimal(new int[] {
            100,
            0,
            0,
            -2147483648});
            this.rate.Name = "rate";
            this.rate.Size = new System.Drawing.Size(120, 22);
            this.rate.TabIndex = 0;
            this.rate.ValueChanged += new System.EventHandler(this.rate_ValueChanged);
            // 
            // expo
            // 
            this.expo.Location = new System.Drawing.Point(341, 22);
            this.expo.Maximum = new decimal(new int[] {
            200,
            0,
            0,
            0});
            this.expo.Minimum = new decimal(new int[] {
            200,
            0,
            0,
            -2147483648});
            this.expo.Name = "expo";
            this.expo.Size = new System.Drawing.Size(120, 22);
            this.expo.TabIndex = 1;
            this.expo.ValueChanged += new System.EventHandler(this.expo_ValueChanged);
            // 
            // graph1
            // 
            this.graph1.Location = new System.Drawing.Point(38, 98);
            this.graph1.Name = "graph1";
            this.graph1.Size = new System.Drawing.Size(572, 572);
            this.graph1.TabIndex = 2;
            // 
            // model
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 16F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(776, 686);
            this.Controls.Add(this.graph1);
            this.Controls.Add(this.expo);
            this.Controls.Add(this.rate);
            this.Name = "model";
            this.Text = "model";
            this.Load += new System.EventHandler(this.model_Load);
            ((System.ComponentModel.ISupportInitialize)(this.rate)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.expo)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.NumericUpDown rate;
        private System.Windows.Forms.NumericUpDown expo;
        private graph graph1;
    }
}