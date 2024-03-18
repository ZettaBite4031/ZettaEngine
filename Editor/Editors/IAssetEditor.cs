using Editor.Content;
using System;

namespace Editor.Editors
{
    enum AssetEditorState
    {
        Done = 0,
        Importing,
        Processing,
        Loading,
        Saving
    }

    interface IAssetEditor
    {
        AssetEditorState State { get; }
        Guid AssetGuid { get; }
        Asset Asset { get; }

        void SetAsset(AssetInfo asset);
    }
}