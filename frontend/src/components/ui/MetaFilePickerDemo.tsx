import { useState } from 'react'
import { MetaFilePicker } from './MetaFilePicker'
import { Button } from './Button'

const MetaFilePickerDemo = () => {
  const [showPicker, setShowPicker] = useState(false)
  const [selectedPath, setSelectedPath] = useState<string>('')
  const [pickerMode, setPickerMode] = useState<'file' | 'directory' | 'both'>('both')

  const handlePathSelect = (path: string) => {
    setSelectedPath(path)
    setShowPicker(false)
    
    // Simulate sending to ProjectManager REST APIs
    console.log('Selected path for ProjectManager:', path)
    
    // Example: Send to analyze archive endpoint
    if (path.endsWith('.7z') || path.endsWith('.zip')) {
      simulateAnalyzeArchive(path)
    }
  }

  const simulateAnalyzeArchive = async (archivePath: string) => {
    try {
      console.log('Sending to ProjectManager /api/projects/analyze:', { archive_path: archivePath })
      
      const response = await fetch('/api/projects/analyze', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          archive_path: archivePath,
          password: '' // Optional password
        })
      })
      
      const result = await response.json()
      console.log('Archive analysis result:', result)
    } catch (error) {
      console.error('Error analyzing archive:', error)
    }
  }

  const handleCancel = () => {
    setShowPicker(false)
  }

  const openPicker = (mode: 'file' | 'directory' | 'both') => {
    setPickerMode(mode)
    setShowPicker(true)
  }

  return (
    <div className="p-6 max-w-4xl mx-auto">
      <h1 className="text-2xl font-bold text-gray-900 mb-6">MetaFilePicker Demo</h1>
      
      <div className="space-y-4 mb-6">
        <div>
          <label className="block text-sm font-medium text-gray-700 mb-2">
            Selected Path:
          </label>
          <div className="p-3 border border-gray-300 rounded-md bg-gray-50 text-sm">
            {selectedPath || 'No path selected'}
          </div>
        </div>
        
        <div className="flex gap-3">
          <Button onClick={() => openPicker('file')}>
            Select File
          </Button>
          
          <Button onClick={() => openPicker('directory')}>
            Select Directory
          </Button>
          
          <Button onClick={() => openPicker('both')}>
            Select File or Directory
          </Button>
          
          {selectedPath && (
            <Button 
              variant="outline" 
              onClick={() => setSelectedPath('')}
            >
              Clear Selection
            </Button>
          )}
        </div>
      </div>

      {showPicker && (
        <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50 p-4">
          <div className="w-full max-w-4xl">
            <MetaFilePicker
              title={
                pickerMode === 'file' ? 'Select File' :
                pickerMode === 'directory' ? 'Select Directory' :
                'Select File or Directory'
              }
              onPathSelect={handlePathSelect}
              onCancel={handleCancel}
              allowFileSelection={pickerMode === 'file' || pickerMode === 'both'}
              allowDirectorySelection={pickerMode === 'directory' || pickerMode === 'both'}
              placeholder={
                pickerMode === 'file' ? 'Browse and select a file...' :
                pickerMode === 'directory' ? 'Browse and select a directory...' :
                'Browse and select a file or directory...'
              }
            />
          </div>
        </div>
      )}
      
      <div className="mt-8 p-4 bg-blue-50 border border-blue-200 rounded-md">
        <h3 className="text-lg font-semibold text-blue-900 mb-2">Integration with ProjectManager</h3>
        <p className="text-blue-800 text-sm mb-3">
          The MetaFilePicker extracts full file paths and can be used with ProjectManager REST endpoints:
        </p>
        <ul className="text-blue-800 text-sm space-y-1">
          <li>• <code>/api/projects/analyze</code> - For analyzing archive files (.7z, .zip)</li>
          <li>• <code>/api/projects/load</code> - For loading projects from archives</li>
          <li>• <code>/api/projects/create-archive</code> - For creating archives from directories</li>
        </ul>
      </div>
    </div>
  )
}

export { MetaFilePickerDemo }