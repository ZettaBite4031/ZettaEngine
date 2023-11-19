﻿using System;
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
    /// Interaction logic for SaveDialog.xaml
    /// </summary>
    public partial class SaveDialog : Window
    {
        public string SaveFilePath { get; private set; }

        public SaveDialog()
        {
            InitializeComponent();
            Closing += OnSaveDialogClosing;
        }

        private bool ValidateFileName(out string saveFilePath)
        {
            var contentBrowser = contentBrowserView.DataContext as ContentBrowser;
            var path = contentBrowser.SelectedFolder;
            if (!Path.EndsInDirectorySeparator(path)) path += @"\";
            var filename = fileNameTextBox.Text.Trim();
            if (string.IsNullOrEmpty(filename))
            {
                saveFilePath = string.Empty;
                return false;
            }

            if (!filename.EndsWith(Asset.AssetFileExtension))
                filename += Asset.AssetFileExtension;

            path += $@"{filename}";
            var isValid = false;
            string errorMsg = string.Empty;

            if (filename.IndexOfAny(Path.GetInvalidFileNameChars()) != -1)
                errorMsg = "Invalid character(s) in asset file name";
            else if (File.Exists(path) &&
                MessageBox.Show("File already exists. Do you wish to overwrite?", "Overwrite file", MessageBoxButton.YesNo, MessageBoxImage.Question) == MessageBoxResult.No) { }
            else isValid = true;

            if (!string.IsNullOrEmpty(errorMsg))
                MessageBox.Show(errorMsg, "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            
            saveFilePath = path;
            return isValid;
        }

        private void OnSave_Button_Click(object sender, RoutedEventArgs e)
        {
            if (ValidateFileName(out var saveFilePath))
            {
                SaveFilePath = saveFilePath;
                DialogResult = true;
                Close();
            }
        }

        private void OnContentBrowser_Mouse_DoubleClick(object sender, MouseButtonEventArgs e)
        {
            if ((e.OriginalSource as FrameworkElement).DataContext == contentBrowserView.SelectedItem &&
                contentBrowserView.SelectedItem.FileName == fileNameTextBox.Text)
            {
                OnSave_Button_Click(sender, null);
            }
        }

        private void OnSaveDialogClosing(object? sender, CancelEventArgs e)
        {
            contentBrowserView.Dispose();
        }
    }
}
