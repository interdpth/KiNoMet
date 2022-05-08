using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace KinometGui
{
    public partial class Form1 : Form
    {
        private string fn;
        public Form1()
        {
            InitializeComponent();
        }

        private void label4_Click(object sender, EventArgs e)
        {

        }

        private void textBox3_TextChanged(object sender, EventArgs e)
        {

        }

        private void button2_Click(object sender, EventArgs e)
        {
            using(OpenFileDialog n = new OpenFileDialog())
            {
                n.Filter = "*.avi";
                n.ShowDialog();
                fn = n.FileName;
            }
        }

        private void Form1_Load(object sender, EventArgs e)
        {

        }

        private void LoadVideoInfo()
        {

        }
    }
}
