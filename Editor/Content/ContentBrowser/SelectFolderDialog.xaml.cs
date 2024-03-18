﻿using Editor.GameProject;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace Editor.Content
{
    /// <summary>
    /// Interaction logic for SelectFolderDialog.xaml
    /// </summary>
    public partial class SelectFolderDialog : Window
    {
        public string SelectedFolder { get; private set; }

        private void OnSelectFolder_Button_Click(object sender, RoutedEventArgs e)
        {
            var contentBrowser = contentBrowserView.DataContext as ContentBrowser;
            SelectedFolder = contentBrowser.SelectedFolder;
            DialogResult = true;
            Close();
        }

        private void OnDialogClosing(object? sender, CancelEventArgs e)
        {
            contentBrowserView.Dispose();
        }

        public SelectFolderDialog(string startFolder)
        {
            InitializeComponent();

            contentBrowserView.Loaded += (_, _) =>
            {
                if (!Path.EndsInDirectorySeparator(startFolder)) startFolder += Path.DirectorySeparatorChar;

                if (startFolder?.Contains(Project.Current.ContentPath) == true)
                {
                    (contentBrowserView.DataContext as ContentBrowser).SelectedFolder = startFolder;
                }
            };

            Closing += OnDialogClosing;
        }
    }
}
