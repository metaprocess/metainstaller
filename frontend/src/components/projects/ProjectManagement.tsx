import { useState } from 'react'
import { useQuery, useMutation, useQueryClient } from '@tanstack/react-query'
import { Play, Square, RotateCcw, Trash2, Upload, Download, Archive, Eye, FileText, Lock } from 'lucide-react'
import { DockerApiService } from '@/services/api'
import { Button, Card, CardHeader, CardTitle, CardContent, Table, TableHeader, TableBody, TableHead, TableRow, TableCell, Badge, Alert, AlertDescription, Input, CardDescription } from '@/components/ui'
import type { ProjectInfo } from '@/types/api'
import { useTestSudo } from '@/hooks/useApi'
import { ArchiveAnalyzer } from './ArchiveAnalyzer'
import { ProjectLoader } from './ProjectLoader'
import { ArchiveCreator } from './ArchiveCreator'
import { ProjectDetails } from './ProjectDetails'

const ProjectManagement = () => {
  const [selectedProject, setSelectedProject] = useState<ProjectInfo | null>(null)
  const [showAnalyzer, setShowAnalyzer] = useState(false)
  const [showLoader, setShowLoader] = useState(false)
  const [showCreator, setShowCreator] = useState(false)
  const [showDetails, setShowDetails] = useState(false)
  const [sudoPassword, setSudoPassword] = useState('')
  const [showPasswordDialog, setShowPasswordDialog] = useState(false)
  const [pendingRemoveProject, setPendingRemoveProject] = useState<{ project: ProjectInfo; removeFiles: boolean } | null>(null)
  const queryClient = useQueryClient()

  const { data: projectsData, isLoading, error } = useQuery({
    queryKey: ['projects'],
    queryFn: DockerApiService.listProjects,
    // refetchInterval: 5000,
  })

  const startProjectMutation = useMutation({
    mutationFn: (projectName: string) => DockerApiService.startProject(projectName),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['projects'] })
    },
  })

  const stopProjectMutation = useMutation({
    mutationFn: (projectName: string) => DockerApiService.stopProject(projectName),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['projects'] })
    },
  })

  const restartProjectMutation = useMutation({
    mutationFn: (projectName: string) => DockerApiService.restartProject(projectName),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['projects'] })
    },
  })

  const unloadProjectMutation = useMutation({
    mutationFn: (projectName: string) => DockerApiService.unloadProject(projectName),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['projects'] })
    },
  })

  const removeProjectMutation = useMutation({
    mutationFn: ({ projectName, removeFiles }: { projectName: string; removeFiles: boolean }) =>
      DockerApiService.removeProject(projectName, { remove_files: removeFiles }),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['projects'] })
    },
  })

  const testSudoMutation = useTestSudo()

  const projects = projectsData?.projects || []

  const getStatusBadge = (project: ProjectInfo) => {
    if (project.is_running)
    {
      return <Badge className="bg-green-100 text-green-800">Running</Badge>
    }
    else
    {
      return <Badge className="bg-red-100 text-red-800">Stopped</Badge>
      // if (project.is_loaded) {
      // return <Badge className="bg-blue-100 text-blue-800">Ready</Badge>
      // } else {
      //   return <Badge className="bg-gray-100 text-gray-800">Not Loaded</Badge>
      // }
    }
  }

  const getLoadedBadge = (project: ProjectInfo) => {
    if (project.is_loaded)
    {
      return <Badge className="bg-blue-100 text-blue-800">Ready</Badge>
    }
    else
    {
      return <Badge className="bg-gray-100 text-gray-800">Not Loaded</Badge>
    }
  }

  const handleViewDetails = (project: ProjectInfo) => {
    setSelectedProject(project)
    setShowDetails(true)
  }

  const handleRemoveProject = (project: ProjectInfo, removeFiles: boolean) => {
    if (window.confirm(`Are you sure you want to remove project "${project.name}"${removeFiles ? ' and delete its files' : ''}?`)) {
      setPendingRemoveProject({ project, removeFiles })
      setShowPasswordDialog(true)
    }
  }

  const handleSudoSubmit = async () => {
    try {
      await testSudoMutation.mutateAsync({ password: sudoPassword })
      setShowPasswordDialog(false)

      if (pendingRemoveProject) {
        removeProjectMutation.mutate({
          projectName: pendingRemoveProject.project.name,
          removeFiles: pendingRemoveProject.removeFiles
        })
        setPendingRemoveProject(null)
      }

      setSudoPassword('')
    } catch (error) {
      console.error('Sudo authentication failed:', error)
    }
  }

  if (error) {
    return (
      <Alert>
        <AlertDescription>
          Failed to load projects: {error instanceof Error ? error.message : 'Unknown error occurred'}
        </AlertDescription>
      </Alert>
    )
  }

  return (
    <div className="space-y-6">
      <div className="flex justify-between items-center">
        <div>
          <h1 className="text-2xl font-bold text-gray-900">MetaProjects</h1>
          <p className="text-gray-600">Manage Docker Compose project archives</p>
        </div>
        <div className="flex gap-2">
          <Button onClick={() => setShowAnalyzer(true)} variant="outline">
            <Eye className="h-4 w-4 mr-2" />
            Analyze Archive
          </Button>
          <Button onClick={() => setShowLoader(true)} variant="outline">
            <Upload className="h-4 w-4 mr-2" />
            Load Project
          </Button>
          <Button onClick={() => setShowCreator(true)} variant="outline">
            <Archive className="h-4 w-4 mr-2" />
            Create Archive
          </Button>
        </div>
      </div>

      <Card>
        <CardHeader>
          <CardTitle>Projects ({projects.length})</CardTitle>
        </CardHeader>
        <CardContent>
          {isLoading ? (
            <div className="flex justify-center py-8">
              <div className="animate-spin rounded-full h-8 w-8 border-b-2 border-blue-600"></div>
            </div>
          ) : projects.length === 0 ? (
            <div className="text-center py-8 text-gray-500">
              <Archive className="h-12 w-12 mx-auto mb-4 text-gray-300" />
              <p>No projects loaded</p>
              <p className="text-sm">Load a project archive to get started</p>
            </div>
          ) : (
            <Table>
              <TableHeader>
                <TableRow>
                  <TableHead>Name</TableHead>
                  <TableHead>Status</TableHead>
                  <TableHead>Loading</TableHead>
                  <TableHead>Services</TableHead>
                  <TableHead>Created</TableHead>
                  <TableHead>Actions</TableHead>
                </TableRow>
              </TableHeader>
              <TableBody>
                {projects.map((project) => (
                  <TableRow key={project.name}>
                    <TableCell className="font-medium">{project.name}</TableCell>
                    <TableCell>{getStatusBadge(project)}</TableCell>
                    <TableCell>{getLoadedBadge(project)}</TableCell>
                    <TableCell>
                      <div className="flex items-center gap-1">
                        <FileText className="h-3 w-3 text-gray-400" />
                        <span>{project.required_images.length} images</span>
                      </div>
                    </TableCell>
                    <TableCell>{new Date(project.created_time).toLocaleDateString()}</TableCell>
                    <TableCell>
                      <div className="flex items-center gap-1">
                        <Button
                          size="sm"
                          variant="ghost"
                          onClick={() => handleViewDetails(project)}
                        >
                          <Eye className="h-3 w-3" />
                        </Button>
                        <Button
                          size="sm"
                          variant="ghost"
                          onClick={() => startProjectMutation.mutate(project.name)}
                          disabled={startProjectMutation.isPending}
                        >
                          <Play className="h-3 w-3" />
                        </Button>
                        <Button
                          size="sm"
                          variant="ghost"
                          onClick={() => stopProjectMutation.mutate(project.name)}
                          disabled={stopProjectMutation.isPending}
                        >
                          <Square className="h-3 w-3" />
                        </Button>
                        <Button
                          size="sm"
                          variant="ghost"
                          onClick={() => restartProjectMutation.mutate(project.name)}
                          disabled={restartProjectMutation.isPending}
                        >
                          <RotateCcw className="h-3 w-3" />
                        </Button>
                        <Button
                          size="sm"
                          variant="ghost"
                          onClick={() => unloadProjectMutation.mutate(project.name)}
                          disabled={unloadProjectMutation.isPending}
                        >
                          <Download className="h-3 w-3" />
                        </Button>
                        <Button
                          size="sm"
                          variant="ghost"
                          onClick={() => handleRemoveProject(project, true)}
                          disabled={removeProjectMutation.isPending}
                          className="text-red-600 hover:text-red-700"
                        >
                          <Trash2 className="h-3 w-3" />
                        </Button>
                      </div>
                    </TableCell>
                  </TableRow>
                ))}
              </TableBody>
            </Table>
          )}
        </CardContent>
      </Card>

      {/* Modals */}
      {showAnalyzer && <ArchiveAnalyzer onClose={() => setShowAnalyzer(false)} />}
      {showLoader && <ProjectLoader onClose={() => setShowLoader(false)} onSuccess={() => {
        setShowLoader(false)
        queryClient.invalidateQueries({ queryKey: ['projects'] })
      }} />}
      {showCreator && <ArchiveCreator onClose={() => setShowCreator(false)} />}
      {showDetails && selectedProject && (
        <ProjectDetails
          project={selectedProject}
          onClose={() => setShowDetails(false)}
        />
      )}

      {/* Sudo Password Dialog */}
      {showPasswordDialog && (
        <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
          <Card className="w-full max-w-md">
            <CardHeader>
              <CardTitle className="flex items-center space-x-2">
                <Lock className="h-5 w-5" />
                <span>Sudo Authentication Required</span>
              </CardTitle>
              <CardDescription>
                Enter your sudo password to remove the project
              </CardDescription>
            </CardHeader>
            <CardContent className="space-y-4">
              <Input
                type="password"
                placeholder="Sudo password"
                value={sudoPassword}
                onChange={(e) => setSudoPassword(e.target.value)}
                onKeyDown={(e) => e.key === 'Enter' && handleSudoSubmit()}
              />

              {!testSudoMutation.error ? null : (
                <Alert variant="destructive">
                  <AlertDescription>
                    Authentication failed. Please check your password.
                  </AlertDescription>
                </Alert>
              )}

              <div className="flex space-x-2 justify-end">
                <Button
                  variant="outline"
                  onClick={() => {
                    setShowPasswordDialog(false)
                    setPendingRemoveProject(null)
                    setSudoPassword('')
                  }}
                >
                  Cancel
                </Button>
                <Button
                  onClick={handleSudoSubmit}
                  loading={testSudoMutation.isPending}
                  disabled={!sudoPassword}
                >
                  Authenticate
                </Button>
              </div>
            </CardContent>
          </Card>
        </div>
      )}
    </div>
  )
}

export default ProjectManagement
