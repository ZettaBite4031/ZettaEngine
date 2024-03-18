﻿using System;
using System.Collections.Generic;
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
using System.Windows.Shapes;

namespace Editor.Content
{
    /// <summary>
    /// Interaction logic for ConfigureGeometryImportSettings.xaml
    /// </summary>
    public partial class ConfigureGeometryImportSettings : UserControl
    {

        private void OnRemoveButton_Click(object sender, RoutedEventArgs e)
        {
            var vm = DataContext as ConfigureImportSettings;
            vm.GeometryImportSettingsConfigurator.RemoveFile((sender as FrameworkElement).DataContext as GeometryProxy);
        }
        
        private void OnImport_Button_Click(object sender, RoutedEventArgs e)
        {
            ((sender as FrameworkElement).DataContext as GeometryImportSettingsConfigurator).Import();
        }

        private void OnApplyToSelection_Button_Click(object sender, RoutedEventArgs e)
        {
            var settings = ((sender as FrameworkElement).DataContext as GeometryProxy).ImportSettings;
            var selection = geometryListBox.SelectedItems;
            foreach (GeometryProxy proxy in selection) proxy.CopySettings(settings);
        }

        private void OnApplyTo_Button_Click(object sender, RoutedEventArgs e)
        {
            var settings = ((sender as FrameworkElement).DataContext as GeometryProxy).ImportSettings;
            var vm = DataContext as ConfigureImportSettings;
            foreach (var proxy in vm.GeometryImportSettingsConfigurator.GeometryProxies) proxy.CopySettings(settings);
        }

        private void OnListBox_Drop(object sender, DragEventArgs e)
        {
            ConfigureImportSettingsWindow.AddDroppedFiles(DataContext as ConfigureImportSettings, sender as ListBox, e);
        }

        public ConfigureGeometryImportSettings()
        {
            InitializeComponent();

            Loaded += (_, _) =>
            {
                var item = geometryListBox.ItemContainerGenerator.ContainerFromItem(geometryListBox.SelectedItem) as ListBoxItem;
                item?.Focus();
            };
        }
    }
}
