/*
 * Created by SharpDevelop.
 * User: Graham
 * Date: 03/11/2009
 * Time: 22:35
 * 
 * To change this template use Tools | Options | Coding | Edit Standard Headers.
 */
namespace Serial
{
    partial class UploadProgress
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
        	this.UploadProgressBar = new System.Windows.Forms.ProgressBar();
        	this.SuspendLayout();
        	// 
        	// UploadProgressBar
        	// 
        	this.UploadProgressBar.Dock = System.Windows.Forms.DockStyle.Fill;
        	this.UploadProgressBar.Location = new System.Drawing.Point(10, 10);
        	this.UploadProgressBar.Name = "UploadProgressBar";
        	this.UploadProgressBar.Size = new System.Drawing.Size(191, 17);
        	this.UploadProgressBar.TabIndex = 0;
        	// 
        	// UploadProgress
        	// 
        	this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
        	this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
        	this.ClientSize = new System.Drawing.Size(211, 37);
        	this.Controls.Add(this.UploadProgressBar);
        	this.MaximizeBox = false;
        	this.MinimizeBox = false;
        	this.Name = "UploadProgress";
        	this.Padding = new System.Windows.Forms.Padding(10);
        	this.Text = "Upload Progress";
        	this.Load += new System.EventHandler(this.UploadProgressLoad);
        	this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.UploadProgressFormClosing);
        	this.ResumeLayout(false);
        }
        private System.Windows.Forms.ProgressBar UploadProgressBar;
    }
}
