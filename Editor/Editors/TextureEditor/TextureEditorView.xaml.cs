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

namespace Editor.Editors
{
    public static class TextureViewCommands
    {
        public static RoutedCommand CenterCommand { get; } = new(nameof(CenterCommand), typeof(TextureEditorView), new() { new KeyGesture(Key.Home) });
        public static RoutedCommand ZoomInCommand { get; } = new(nameof(ZoomInCommand), typeof(TextureEditorView), new() { new KeyGesture(Key.OemPlus, ModifierKeys.Control) });
        public static RoutedCommand ZoomOutCommand { get; } = new(nameof(ZoomOutCommand), typeof(TextureEditorView), new() { new KeyGesture(Key.OemMinus, ModifierKeys.Control) });
        public static RoutedCommand ZoomFitCommand { get; } = new(nameof(ZoomFitCommand), typeof(TextureEditorView), new() { new KeyGesture(Key.D0, ModifierKeys.Alt) });
        public static RoutedCommand ActualSizeCommand { get; } = new(nameof(ActualSizeCommand), typeof(TextureEditorView), new() { new KeyGesture(Key.D0, ModifierKeys.Control) });
    }

    /// <summary>
    /// Interaction logic for TextureEditorView.xaml
    /// </summary>
    public partial class TextureEditorView : UserControl
    {
        private void OnCenterTexture(object sender, ExecutedRoutedEventArgs e) => textureView.Center();

        private void OnZoomInTexture(object sender, ExecutedRoutedEventArgs e) => textureView.ZoomIn();

        private void OnZoomOutTexture(object sender, ExecutedRoutedEventArgs e) => textureView.ZoomOut();

        private void OnZoomFitTexture(object sender, ExecutedRoutedEventArgs e) => textureView.ZoomFit();

        private void OnActualSizeTexture(object sender, ExecutedRoutedEventArgs e) => textureView.ActualSize();
        
        public TextureEditorView()
        {
            InitializeComponent();
            Focus();
        }
    }
}
