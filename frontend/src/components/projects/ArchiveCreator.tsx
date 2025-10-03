import { useState } from 'react'
import { useMutation } from '@tanstack/react-query'
import { X, Archive, CheckCircle, XCircle, FolderOpen } from 'lucide-react'
import { DockerApiService } from '@/services/api'
import { Button, Card, CardHeader, CardTitle, CardContent, Input, MetaFilePicker, Alert, AlertDescription } from '@/components/ui'

interface ArchiveCreatorProps {
  onClose: () => void
}

export const ArchiveCreator = ({ onClose }: ArchiveCreatorProps) => {
  const [projectPath, setProjectPath] = useState('')
  const [archivePath, setArchivePath] = useState('')
  const [password, setPassword] = useState('')
  const [showProjectPicker, setShowProjectPicker] = useState(false)
  const [showArchivePicker, setShowArchivePicker] = useState(false)

  const createArchiveMutation = useMutation({
    mutationFn: (data: { project_path: string; archive_path: string; password?: string }) => 
      DockerApiService.createProjectArchive(data),
  })

  const handleProjectPathSelect = (path: string) => {
    setProjectPath(path)
    setShowProjectPicker(false)
    
    // Auto-suggest archive name
    if (!archivePath) {
      const folderName = path.split('/').pop() || 'project'
      setArchivePath(`${folderName}.7z`)
    }
  }

  const handleArchivePathSelect = (path: string) => {
    setArchivePath(path)
    setShowArchivePicker(false)
  }

  const handleArchivePathChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    setArchivePath(e.target.value)
  }

  const handleCreate = () => {
    if (!projectPath || !archivePath) return
    
    createArchiveMutation.mutate({
      project_path: projectPath,
      archive_path: archivePath,
      password: password || undefined
    })
  }

  return (
    <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
      <div className="bg-white rounded-lg shadow-xl max-w-lg w-full mx-4">
        <div className="flex items-center justify-between p-6 border-b">
          <h2 className="text-xl font-semibold">Create Archive</h2>
          <Button variant="ghost" size="sm" onClick={onClose}>
            <X className="h-4 w-4" />
          </Button>
        </div>

        <div className="p-6 space-y-6">
          <div className="space-y-4">
            <div>
              <label className="block text-sm font-medium mb-2">
                Project Directory <span className="text-red-500">*</span>
              </label>
              <div className="flex gap-2">
                <Input
                  value={projectPath}
                  onChange={(e) => setProjectPath(e.target.value)}
                  placeholder="Enter project directory path"
                  className="flex-1"
                />
                <Button
                  type="button"
                  variant="outline"
                  onClick={() => setShowProjectPicker(true)}
                >
                  <FolderOpen className="h-4 w-4 mr-1" />
                  Browse
                </Button>
                {projectPath && (
                  <Button
                    type="button"
                    variant="ghost"
                    size="sm"
                    onClick={() => setProjectPath('')}
                  >
                    <X className="h-4 w-4" />
                  </Button>
                )}
              </div>
              <p className="text-xs text-gray-500 mt-1">
                Click Browse to select a directory or enter the full path manually
              </p>
            </div>

            <div>
              <label className="block text-sm font-medium mb-2">
                Archive Path <span className="text-red-500">*</span>
              </label>
              <div className="flex gap-2">
                <Input
                  value={archivePath}
                  onChange={handleArchivePathChange}
                  placeholder="project-archive.7z"
                  className="flex-1"
                />
                <Button
                  type="button"
                  variant="outline"
                  onClick={() => setShowArchivePicker(true)}
                >
                  Browse
                </Button>
                {archivePath && (
                  <Button
                    type="button"
                    variant="ghost"
                    size="sm"
                    onClick={() => setArchivePath('')}
                  >
                    <X className="h-4 w-4" />
                  </Button>
                )}
              </div>
              <p className="text-xs text-gray-500 mt-1">
                Supported formats: .7z (recommended), .zip, .tar, .tar.gz
              </p>
            </div>

            <div>
              <label className="block text-sm font-medium mb-2">Password (optional)</label>
              <Input
                type="password"
                value={password}
                onChange={(e) => setPassword(e.target.value)}
                placeholder="Enter password to encrypt archive"
              />
            </div>

            <Alert>
              <AlertDescription>
                This will create a compressed archive containing all files from the project directory. Include docker-compose.yml and any Docker image tar files for a complete project package.
              </AlertDescription>
            </Alert>

            <Button
              onClick={handleCreate}
              disabled={!projectPath || !archivePath || createArchiveMutation.isPending}
              loading={createArchiveMutation.isPending}
              className="w-full"
            >
              <Archive className="h-4 w-4 mr-2" />
              Create Archive
            </Button>
          </div>

          {createArchiveMutation.error && (
            <Alert>
              <XCircle className="h-4 w-4" />
              <AlertDescription>
                Failed to create archive: {createArchiveMutation.error.message}
              </AlertDescription>
            </Alert>
          )}

          {createArchiveMutation.isSuccess && (
            <Alert>
              <CheckCircle className="h-4 w-4" />
              <AlertDescription>
                Archive created successfully at: {archivePath}
              </AlertDescription>
            </Alert>
          )}
        </div>

        <div className="flex justify-end gap-2 p-6 border-t">
          <Button variant="outline" onClick={onClose}>
            {createArchiveMutation.isSuccess ? 'Close' : 'Cancel'}
          </Button>
        </div>
      </div>

      {showProjectPicker && (
        <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-[60]">
          <div className="w-full max-w-4xl p-4">
            <MetaFilePicker
              title="Select Project Directory"
              onPathSelect={handleProjectPathSelect}
              onCancel={() => setShowProjectPicker(false)}
              allowFileSelection={false}
              allowDirectorySelection={true}
              placeholder="Browse and select the project directory to archive..."
            />
          </div>
        </div>
      )}

      {showArchivePicker && (
        <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-[60]">
          <div className="w-full max-w-4xl p-4">
            <MetaFilePicker
              title="Select Archive Location"
              onPathSelect={handleArchivePathSelect}
              onCancel={() => setShowArchivePicker(false)}
              allowFileSelection={true}
              allowDirectorySelection={true}
              placeholder="Browse and select where to save the archive file..."
            />
          </div>
        </div>
      )}
    </div>
  )
}