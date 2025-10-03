import { useState } from 'react'
import { useMutation } from '@tanstack/react-query'
import { X, Upload, CheckCircle, XCircle } from 'lucide-react'
import { DockerApiService } from '@/services/api'
import { Button, Card, CardHeader, CardTitle, CardContent, Input, MetaFilePicker, Alert, AlertDescription } from '@/components/ui'

interface ProjectLoaderProps {
  onClose: () => void
  onSuccess: () => void
}

export const ProjectLoader = ({ onClose, onSuccess }: ProjectLoaderProps) => {
  const [archivePath, setArchivePath] = useState('')
  const [projectName, setProjectName] = useState('')
  const [password, setPassword] = useState('')
  const [showFilePicker, setShowFilePicker] = useState(false)

  const loadProjectMutation = useMutation({
    mutationFn: (data: { archive_path: string; project_name: string; password?: string }) => 
      DockerApiService.loadProject(data),
    onSuccess: () => {
      onSuccess()
    },
  })

  const handlePathSelect = (path: string) => {
    setArchivePath(path)
    setShowFilePicker(false)
    
    // Auto-suggest project name from filename
    const fileName = path.split('/').pop()?.replace(/\.[^/.]+$/, '') || '' // Remove extension
    const sanitizedName = fileName.replace(/[^a-zA-Z0-9-_]/g, '').toLowerCase()
    if (sanitizedName && !projectName) {
      setProjectName(sanitizedName)
    }
  }

  const handleLoad = () => {
    if (!archivePath || !projectName) return
    
    loadProjectMutation.mutate({
      archive_path: archivePath,
      project_name: projectName,
      password: password || undefined
    })
  }

  const isValidProjectName = (name: string) => {
    return /^[a-zA-Z0-9-_]+$/.test(name) && name.length <= 64 && name.length > 0
  }

  return (
    <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
      <div className="bg-white rounded-lg shadow-xl max-w-lg w-full mx-4">
        <div className="flex items-center justify-between p-6 border-b">
          <h2 className="text-xl font-semibold">Load Project</h2>
          <Button variant="ghost" size="sm" onClick={onClose}>
            <X className="h-4 w-4" />
          </Button>
        </div>

        <div className="p-6 space-y-6">
          <div className="space-y-4">
            <div>
              <label className="block text-sm font-medium mb-2">
                Archive File <span className="text-red-500">*</span>
              </label>
              <div className="flex gap-2">
                <Input
                  value={archivePath}
                  onChange={(e) => setArchivePath(e.target.value)}
                  placeholder="Select project archive file"
                  className="flex-1"
                />
                <Button
                  type="button"
                  variant="outline"
                  onClick={() => setShowFilePicker(true)}
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
            </div>

            <div>
              <label className="block text-sm font-medium mb-2">
                Project Name <span className="text-red-500">*</span>
              </label>
              <Input
                value={projectName}
                onChange={(e) => setProjectName(e.target.value)}
                placeholder="Enter unique project name"
                className={!isValidProjectName(projectName) && projectName ? 'border-red-300' : ''}
              />
              {projectName && !isValidProjectName(projectName) && (
                <p className="text-xs text-red-600 mt-1">
                  Project name must contain only letters, numbers, hyphens, and underscores (max 64 characters)
                </p>
              )}
            </div>

            <div>
              <label className="block text-sm font-medium mb-2">Password (optional)</label>
              <Input
                type="password"
                value={password}
                onChange={(e) => setPassword(e.target.value)}
                placeholder="Enter password if archive is encrypted"
              />
            </div>

            <Alert>
              <AlertDescription>
                Loading a project will extract the archive, validate the docker-compose file, and load any Docker images included in the archive.
              </AlertDescription>
            </Alert>

            <Button
              onClick={handleLoad}
              disabled={!archivePath || !projectName || !isValidProjectName(projectName) || loadProjectMutation.isPending}
              loading={loadProjectMutation.isPending}
              className="w-full"
            >
              <Upload className="h-4 w-4 mr-2" />
              Load Project
            </Button>
          </div>

          {loadProjectMutation.error && (
            <Alert>
              <XCircle className="h-4 w-4" />
              <AlertDescription>
                Failed to load project: {loadProjectMutation.error.message}
              </AlertDescription>
            </Alert>
          )}

          {loadProjectMutation.isSuccess && (
            <Alert>
              <CheckCircle className="h-4 w-4" />
              <AlertDescription>
                Project loaded successfully!
              </AlertDescription>
            </Alert>
          )}
        </div>

        <div className="flex justify-end gap-2 p-6 border-t">
          <Button variant="outline" onClick={onClose}>
            Cancel
          </Button>
        </div>
      </div>

      {showFilePicker && (
        <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-[60]">
          <div className="w-full max-w-4xl p-4">
            <MetaFilePicker
              title="Select Project Archive"
              onPathSelect={handlePathSelect}
              onCancel={() => setShowFilePicker(false)}
              allowFileSelection={true}
              allowDirectorySelection={false}
              placeholder="Browse and select a project archive file (.7z)"
            />
          </div>
        </div>
      )}
    </div>
  )
}