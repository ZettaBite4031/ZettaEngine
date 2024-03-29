﻿using Editor.GameProject;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
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
using System.Windows.Navigation;

namespace Editor.Content
{
    class ContentSubfolderConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            var contentFolder = Project.Current.ContentPath;
            if (value is string folder && !string.IsNullOrEmpty(folder) && folder.Contains(contentFolder))
            {
                return $@"{Path.DirectorySeparatorChar}{folder.Replace(contentFolder, "")}";
            }
            return null;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    /// <summary>
    /// Interaction logic for ChangeDestinationFolder.xaml
    /// </summary>
    public partial class ChangeDestinationFolder : UserControl
    {
        public ChangeDestinationFolder()
        {
            InitializeComponent();
        }

        private void OnChangeDestinationFolder_Button_Click(object sender, RoutedEventArgs e)
        {
            var proxy = (sender as Button).DataContext as AssetProxy;
            var dstFolder = proxy.DstFolder;
            if (Path.EndsInDirectorySeparator(dstFolder)) dstFolder = Path.GetDirectoryName(dstFolder);

            var dlg = new SelectFolderDialog(dstFolder);

            if (dlg.ShowDialog() == true)
            {
                Debug.Assert(!string.IsNullOrEmpty(dlg.SelectedFolder));
                proxy.DstFolder = dlg.SelectedFolder;
            }
        }
    }
}
